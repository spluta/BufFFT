/*
    Copyright (c) 2023 Sam Pluta
    Built upon the improved FFT and IFFT UGens for SuperCollider 3
    Copyright (c) 2007-2008 Dan Stowell, incorporating code from
    SuperCollider 3 Copyright (c) 2002 James McCartney.
    All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "FFT_UGens.h"
#include "SC_PlugIn.h"
#include "SC_PlugIn.hpp"


InterfaceTable *ft;

struct BufFFTBase : Unit {
    SndBuf* m_fftsndbuf;
    float* m_fftbuf;

    int m_pos, m_fullbufsize, m_audiosize; // "fullbufsize" includes any zero-padding, "audiosize" does not.
    int m_log2n_full, m_log2n_audio;

    uint32 m_fftbufnum;

    scfft* m_scfft;

    int m_hopsize, m_shuntsize; // These add up to m_audiosize
    int m_wintype;

    int m_numSamples;
};

struct BufFFT : public BufFFTBase {
    float* m_inbuf;
    float m_prevtrig;
    // float m_fbufnum;
    // SndBuf* m_buf;
};

struct BufIFFT : public BufFFTBase {
    float* m_olabuf;
    int m_numSamples;
};

struct BufFFTTrigger : public BufFFTBase {
    int m_numPeriods, m_periodsRemain, m_polar, m_count, m_skipcount;
};

struct BufFFTTrigger2 : public BufFFTBase {

};

struct BufFFT_BufCopy : public PV_Unit {
    
};

struct PV_AccumPhase : public PV_Unit {
    
};


//////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" {
void BufFFT_Ctor(BufFFT* unit);
void BufFFT_ClearUnitOutputs(BufFFT* unit, int wrongNumSamples);
void BufFFT_next(BufFFT* unit, int inNumSamples);
void BufFFT_Dtor(BufFFT* unit);

void BufIFFT_Ctor(BufIFFT* unit);
void BufIFFT_next(BufIFFT* unit, int inNumSamples);
void BufIFFT_Dtor(BufIFFT* unit);

void BufFFT_BufCopy_Ctor(BufFFT_BufCopy* unit);
void BufFFT_BufCopy_next(BufFFT_BufCopy* unit, int inNumSamples);

void PV_AccumPhase_Ctor(PV_Unit* unit);
void PV_AccumPhase_next(PV_Unit* unit, int inNumSamples);

void BufFFTTrigger_Ctor(BufFFTTrigger* unit);
void BufFFTTrigger_next(BufFFTTrigger* unit, int inNumSamples);

void BufFFTTrigger2_Ctor(BufFFTTrigger2* unit);
void BufFFTTrigger2_next(BufFFTTrigger2* unit, int inNumSamples);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static int BufFFTBase_Ctor(BufFFTBase* unit, int frmsizinput) {
    World* world = unit->mWorld;

    uint32 bufnum = (uint32)ZIN0(0);
    SndBuf* buf;
    if (bufnum >= world->mNumSndBufs) {
        int localBufNum = bufnum - world->mNumSndBufs;
        Graph* parent = unit->mParent;
        if (localBufNum <= parent->localMaxBufNum) {
            buf = parent->mLocalSndBufs + localBufNum;
        } else {
            if (unit->mWorld->mVerbosity > -1) {
                Print("FFTBase_Ctor error: invalid buffer number: %i.\n", bufnum);
            }
            return 0;
        }
    } else {
        buf = world->mSndBufs + bufnum;
    }

    if (!buf->data) {
        if (unit->mWorld->mVerbosity > -1) {
            Print("FFTBase_Ctor error: Buffer %i not initialised.\n", bufnum);
        }
        return 0;
    }

    unit->m_fftsndbuf = buf;
    unit->m_fftbufnum = bufnum;
    unit->m_fullbufsize = buf->samples;
    int framesize = (int)ZIN0(frmsizinput);
    if (framesize < 1)
        unit->m_audiosize = buf->samples;
    else
        unit->m_audiosize = sc_min(buf->samples, framesize);

    unit->m_log2n_full = LOG2CEIL(unit->m_fullbufsize);
    unit->m_log2n_audio = LOG2CEIL(unit->m_audiosize);


    //Although FFTW allows non-power-of-two buffers (vDSP doesn't), this would complicate the windowing, so we don't
    //allow it.
    if (!ISPOWEROFTWO(unit->m_fullbufsize)) {
        Print("FFTBase_Ctor error: buffer size (%i) not a power of two.\n", unit->m_fullbufsize);
        return 0;
    } else if (!ISPOWEROFTWO(unit->m_audiosize)) {
        Print("FFTBase_Ctor error: audio frame size (%i) not a power of two.\n", unit->m_audiosize);
        return 0;
    } else if (unit->m_audiosize < SC_FFT_MINSIZE
               || (((int)(unit->m_audiosize / unit->mWorld->mFullRate.mBufLength)) * unit->mWorld->mFullRate.mBufLength
                   != unit->m_audiosize)) {
        Print("FFTBase_Ctor error: audio frame size (%i) not a multiple of the block size (%i).\n", unit->m_audiosize,
              unit->mWorld->mFullRate.mBufLength);
        return 0;
    }

    unit->m_pos = 0;

    ZOUT0(0) = ZIN0(0);

    return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////



void BufFFT_Ctor(BufFFT* unit) {
    int winType = sc_clip((int)ZIN0(1), -1, 1); // wintype may be used by the base ctor
    unit->m_wintype = winType;
    // These zeroes are to prevent the dtor freeing things that don't exist:
    unit->m_inbuf = nullptr;
    unit->m_scfft = nullptr;
    unit->m_prevtrig = 0.f;


    if (!BufFFTBase_Ctor(unit, 2)) {
        SETCALC(FFT_ClearUnitOutputs);
        return;
    }
    int audiosize = unit->m_audiosize * sizeof(float);

    unit->m_inbuf = (float*)RTAlloc(unit->mWorld, audiosize);
    ClearFFTUnitIfMemFailed(unit->m_inbuf);

    SCWorld_Allocator alloc(ft, unit->mWorld);
    unit->m_scfft = scfft_create(unit->m_fullbufsize, unit->m_audiosize, (SCFFT_WindowFunction)unit->m_wintype,
                                 unit->m_inbuf, unit->m_fftsndbuf->data, kForward, alloc);
    ClearFFTUnitIfMemFailed(unit->m_scfft);

    memset(unit->m_inbuf, 0, audiosize);

    if (INRATE(1) == calc_FullRate) {
        unit->m_numSamples = unit->mWorld->mFullRate.mBufLength;
    } else {
        unit->m_numSamples = 1;
    }

    SETCALC(BufFFT_next);
}

void BufFFT_Dtor(BufFFT* unit) {
    SCWorld_Allocator alloc(ft, unit->mWorld);
    if (unit->m_scfft)
        scfft_destroy(unit->m_scfft, alloc);

    if (unit->m_inbuf)
        RTFree(unit->mWorld, unit->m_inbuf);
}

void BufFFT_next(BufFFT* unit, int wrongNumSamples) {
    float* out = unit->m_inbuf;
    
    float fbufnum = ZIN0(0);
    float wintype = ZIN0(1);
    float winsize = ZIN0(2);

    //Print("%f \n", 1.f);

    //do nothing if the buffer number is less than 0
    if (fbufnum < 0.f) {
        ZOUT0(0) = -1.f;
        //Print("%f \n", -1.f);
        return;
    }

    //Print("do it %f \n", fbufnum);
    uint32 ibufnum1 = (int)fbufnum;
    World* world = unit->mWorld;
    SndBuf* buf;
    if (ibufnum1 >= world->mNumSndBufs) {
        int localBufNum = ibufnum1 - world->mNumSndBufs;
        Graph* parent = unit->mParent;
        if (localBufNum <= parent->localBufNum) {
            buf = parent->mLocalSndBufs + localBufNum;
        } else {
            buf = world->mSndBufs;
        }
    } else {
        buf = world->mSndBufs + ibufnum1;
    }

    //Print("%i \n", buf->frames);
    memcpy(out, buf->data, buf->frames * sizeof(float));

    scfft_dofft(unit->m_scfft);
    unit->m_fftsndbuf->coord = coord_Complex;

    ZOUT0(0) = fbufnum;

    RELEASE_SNDBUF_SHARED(buf);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufIFFT_Ctor(BufIFFT* unit) {
    int winType = sc_clip((int)ZIN0(1), -1, 1); // wintype may be used by the base ctor
    unit->m_wintype = winType;
    // These zeroes are to prevent the dtor freeing things that don't exist:
    unit->m_olabuf = nullptr;
    unit->m_scfft = nullptr;

    if (!BufFFTBase_Ctor(unit, 2)) {
        SETCALC(*ClearUnitOutputs);
        return;
    }

    // This will hold the transformed and progressively overlap-added data ready for outputting.
    unit->m_olabuf = (float*)RTAlloc(unit->mWorld, unit->m_audiosize * sizeof(float));
    ClearUnitIfMemFailed(unit->m_olabuf);
    memset(unit->m_olabuf, 0, unit->m_audiosize * sizeof(float));

    SCWorld_Allocator alloc(ft, unit->mWorld);
    unit->m_scfft = scfft_create(unit->m_fullbufsize, unit->m_audiosize, (SCFFT_WindowFunction)unit->m_wintype,
                                 unit->m_fftsndbuf->data, unit->m_fftsndbuf->data, kBackward, alloc);
    ClearUnitIfMemFailed(unit->m_scfft);

    // "pos" will be reset to zero when each frame comes in. Until then, the following ensures silent output at first:
    unit->m_pos = 0; // unit->m_audiosize;

    if (unit->mCalcRate == calc_FullRate) {
        unit->m_numSamples = unit->mWorld->mFullRate.mBufLength;
    } else {
        unit->m_numSamples = 1;
    }

    SETCALC(BufIFFT_next);
    ClearUnitOutputs(unit, 1);
}

void BufIFFT_Dtor(BufIFFT* unit) {
    if (unit->m_olabuf)
        RTFree(unit->mWorld, unit->m_olabuf);

    SCWorld_Allocator alloc(ft, unit->mWorld);
    if (unit->m_scfft)
        scfft_destroy(unit->m_scfft, alloc);
}

void BufIFFT_next(BufIFFT* unit, int wrongNumSamples) {
    float* out = OUT(0); // NB not ZOUT0

    // Load state from struct into local scope
    int pos = unit->m_pos;
    int audiosize = unit->m_audiosize;

    float* olabuf = unit->m_olabuf;
    float fbufnum = ZIN0(0);

    int numSamples = unit->m_numSamples;

    //Print("%i \n", audiosize);

    //Print("%f \n", fbufnum);
    // Only run the BufIFFT if we're receiving a new block of input data - otherwise just output data already received
    if (fbufnum >= 0.f) {
        // Ensure it's in cartesian format, not polar
        ToComplexApx(unit->m_fftsndbuf);

        scfft_doifft(unit->m_scfft);

        // Move the pointer back to zero, which is where playback will next begin
        pos = 0;

    } // End of has-the-chain-fired
   
    if (pos >= audiosize)
        ClearUnitOutputs(unit, numSamples);
    else {
        memcpy(out, unit->m_fftsndbuf->data+pos, numSamples * sizeof(float));
        pos += numSamples;
    }

    unit->m_pos = pos;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufFFTTrigger_Ctor(BufFFTTrigger* unit) {
    World* world = unit->mWorld;

    uint32 bufnum = (uint32)IN0(0);
    // Print("BufFFTTrigger_Ctor: bufnum is %i\n", bufnum);
    SndBuf* buf;
    if (bufnum >= world->mNumSndBufs) {
        int localBufNum = bufnum - world->mNumSndBufs;
        Graph* parent = unit->mParent;
        if (localBufNum <= parent->localMaxBufNum) {
            buf = parent->mLocalSndBufs + localBufNum;
        } else {
            bufnum = 0;
            buf = world->mSndBufs + bufnum;
        }
    } else {
        buf = world->mSndBufs + bufnum;
    }
    LOCK_SNDBUF(buf);


    unit->m_fftsndbuf = buf;
    unit->m_fftbufnum = bufnum;
    unit->m_fullbufsize = buf->samples;

    int numSamples = unit->mWorld->mFullRate.mBufLength;


    
    float dataHopSize = IN0(1);
    unit->m_count = (int)IN0(2);
    unit->m_skipcount = (int)IN0(3);

    unit->m_numPeriods = unit->m_periodsRemain = (int)(((float)unit->m_fullbufsize * dataHopSize) / numSamples) - 1;

    unit->m_periodsRemain = 0;
    //Print("%f \n", dataHopSize);

    buf->coord = (IN0(4) == 1.f) ? coord_Polar : coord_Complex;

    OUT0(0) = IN0(0);
    SETCALC(BufFFTTrigger_next);
}

void BufFFTTrigger_next(BufFFTTrigger* unit, int inNumSamples) {
    if (unit->m_periodsRemain > 0) {
        ZOUT0(0) = -1.f;
        unit->m_periodsRemain--;
    } else {
      if(unit->m_count>0){
        ZOUT0(0) = -1.f;
        unit->m_count--;
      } else {
        unit->m_count = unit->m_skipcount-1;
        ZOUT0(0) = unit->m_fftbufnum;
      }
      unit->m_periodsRemain = unit->m_numPeriods;
    }    
  }



void BufFFTTrigger2_Ctor(BufFFTTrigger2* unit) {
    World* world = unit->mWorld;

    uint32 bufnum = (uint32)IN0(0);
    // Print("BufFFTTrigger_Ctor: bufnum is %i\n", bufnum);
    SndBuf* buf;
    if (bufnum >= world->mNumSndBufs) {
        int localBufNum = bufnum - world->mNumSndBufs;
        Graph* parent = unit->mParent;
        if (localBufNum <= parent->localMaxBufNum) {
            buf = parent->mLocalSndBufs + localBufNum;
        } else {
            bufnum = 0;
            buf = world->mSndBufs + bufnum;
        }
    } else {
        buf = world->mSndBufs + bufnum;
    }
    LOCK_SNDBUF(buf);

    buf->coord = (IN0(2) == 1.f) ? coord_Polar : coord_Complex;

    unit->m_fftsndbuf = buf;
    unit->m_fftbufnum = bufnum;
    unit->m_fullbufsize = buf->samples;
    OUT0(0) = IN0(0);
    SETCALC(BufFFTTrigger2_next);

}

void BufFFTTrigger2_next(BufFFTTrigger2* unit, int inNumSamples) {
    float fbufnum1 = ZIN0(0);
    float trig = IN0(1);
    
    if (trig > 0.f) {
        ZOUT0(0) = unit->m_fftbufnum;;
    } else {
        ZOUT0(0) = -1.f;  
    }
}

void BufFFT_BufCopy_next(BufFFT_BufCopy* unit, int inNumSamples) {
    float fbufnum1 = ZIN0(0); //chainBuf
    float fbufnum2 = ZIN0(1); //sourceBuf
    float start_frame = ZIN0(2);
    float rateIn = IN0(3);

    //do nothing if the buffer number is less than 0
    if (fbufnum1 < 0.f) {
        ZOUT0(0) = -1.f;
        return;
    }

    ZOUT0(0) = fbufnum1;
    
    uint32 ibufnum1 = (int)fbufnum1;
    uint32 ibufnum2 = (int)fbufnum2;
    World* world = unit->mWorld;
    SndBuf* chainBuf;
    SndBuf* sourceBuf;
    if (ibufnum1 >= world->mNumSndBufs) {
        int localBufNum = ibufnum1 - world->mNumSndBufs;
        Graph* parent = unit->mParent;
        if (localBufNum <= parent->localBufNum) {
            chainBuf = parent->mLocalSndBufs + localBufNum;
        } else {
            chainBuf = world->mSndBufs;
        }
    } else {
        chainBuf = world->mSndBufs + ibufnum1;
    }
    if (ibufnum2 >= world->mNumSndBufs) {
        int localBufNum = ibufnum2 - world->mNumSndBufs;
        Graph* parent = unit->mParent;
        if (localBufNum <= parent->localBufNum) {
            sourceBuf = parent->mLocalSndBufs + localBufNum;
        } else {
            sourceBuf = world->mSndBufs;
        }
    } else {
        sourceBuf = world->mSndBufs + ibufnum2;
    }

    int numChans = sourceBuf->channels;
    //copy the range of the buffer -- if trying to copy beyond the buffer, copy 0
    
    //Print("%i \n", chainBuf->samples);

    for (int i=0; i<chainBuf->samples; i++){
        //if given a multichannel buffer, it will only copy the first channel
        float buf_point = ((float)i*rateIn)+start_frame*numChans;

        long bp1 = long(floor(buf_point));

        //quadratic interpolation on the copy
        if(bp1<sourceBuf->frames){
            float fracphase = buf_point - floor(buf_point); 

            long bp0 = sc_clip(bp1-numChans, 0, sourceBuf->samples-1);
            long bp2 = sc_clip(bp1+numChans, 0, sourceBuf->samples-1);
            long bp3 = sc_clip(bp1+2*numChans, 0, sourceBuf->samples-1);

            chainBuf->data[i] = cubicinterp(fracphase, sourceBuf->data[bp0], sourceBuf->data[bp1], sourceBuf->data[bp2], sourceBuf->data[bp3]); 
        
        } else { 
            chainBuf->data[i]=0.f; 
        }
    

    }
    RELEASE_SNDBUF_SHARED(chainBuf);
    RELEASE_SNDBUF_SHARED(sourceBuf);
}

void BufFFT_BufCopy_Ctor(BufFFT_BufCopy* unit) {
    SETCALC(BufFFT_BufCopy_next);
    ZOUT0(0) = ZIN0(0);
}


void PV_AccumPhase_next(PV_Unit* unit, int inNumSamples) {
    PV_GET_BUF2

    SCPolarBuf* p = ToPolarApx(buf1);
    SCPolarBuf* q = ToPolarApx(buf2);

    // if ((p->dc > 0.f) == (q->dc < 0.f))
    //     p->dc = -p->dc;
    // if ((p->nyq > 0.f) == (q->nyq < 0.f))
    //     p->nyq = -p->nyq;
    for (int i = 0; i < numbins; ++i) {
        p->bin[i].phase += q->bin[i].phase;
        if(p->bin[i].phase>pi) {p->bin[i].phase = p->bin[i].phase-twopi;}
    }
}

void PV_AccumPhase_Ctor(PV_Unit* unit) {
    SETCALC(PV_AccumPhase_next);
    ZOUT0(0) = ZIN0(0);
}



#define DefinePVUnit(name) (*ft->fDefineUnit)(#name, sizeof(PV_Unit), (UnitCtorFunc)&name##_Ctor, 0, 0);


PluginLoad(BufFFT)
{
    // InterfaceTable *inTable implicitly given as argument to the load function
    ft = inTable; // store pointer to InterfaceTable
    //init_SCComplex(inTable);

    DefineDtorUnit(BufFFT);
    DefineDtorUnit(BufIFFT);
    DefinePVUnit(BufFFT_BufCopy);
    DefinePVUnit(PV_AccumPhase);
    DefineSimpleUnit(BufFFTTrigger);
    DefineSimpleUnit(BufFFTTrigger2);
}