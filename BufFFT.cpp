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

#if defined(__APPLE__) && !defined(SC_IPHONE)
    #include <Accelerate/Accelerate.h>
#endif


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

struct BufFFTBase2 : BufFFTBase {
    SndBuf* m_fftsndbuf2;
    float* m_fftbuf2;

    uint32 m_fftbufnum2;

    scfft* m_scfft2;
};

struct BufFFT : public BufFFTBase {
    float* m_inbuf;
    float m_prevtrig;
};

struct BufIFFT : public BufFFTBase {
    int m_numSamples;
};

struct BufIFFT2 : public BufFFTBase2 {
    float* m_olabuf;
    float* m_winbuf;
    int m_numSamples;
};

// struct BufFFT_CrossFade : public BufFFTBase {
//     float* m_winbuf;
//     int m_numSamples;
//     float* m_outbuf;
//     int m_audiosizeDiv2;
// };

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
//constants and functions

constexpr double PI = 3.14159265358979323846;

//uses the correlation value to calculate an overlap window that will smooth the crossfade
//bringing up an amplitude trough and lowering an amplitude peak 
void make_ness_window(float *window, int len, float correlation) {
    //correlation = abs(correlation);

    int halflen = len/2;

   float *floats = (float *)malloc(halflen * sizeof(float));

    for (int iter = 0; iter < halflen; iter++) {
    //for (int iter = 0; iter < len; iter++) {
        floats[iter] = (float)iter / ((float)len / 2.0f);
        
    }

    for (int iter = 0; iter < halflen; ++iter) {
        float fs = powf(tanf(floats[iter] * PI / 2.0f), 2.0f);
        window[iter] = fs * sqrtf(1.0f / (1.0f + (2.0f * fs * correlation) + powf(fs, 2.0f)));
    }
}


float calculate_correlation(float *endLast, float *startNext, int half_len) {

    float sum_x = 0.0f, sum_xx = 0.0f, sum_xy = 0.0f;

    for (int i = 0; i < half_len; i++) {
        sum_x += endLast[i];
        sum_xy += endLast[i]*startNext[i];
        sum_xx += endLast[i] * endLast[i];
    }

    if (sum_x == 0.0f || sum_xx == 0.0f )
        return 0.0f;
    else {
        //return sc_clip(abs(sum_xy/sum_xx), 0.f, 1.f);
        return abs(sum_xy/sum_xx);
    } 
}



//////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" {
void BufFFT_Ctor(BufFFT* unit);
void BufFFT_ClearUnitOutputs(BufFFT* unit, int wrongNumSamples);
void BufFFT_next(BufFFT* unit, int inNumSamples);
void BufFFT_Dtor(BufFFT* unit);

void BufIFFT_Ctor(BufIFFT* unit);
void BufIFFT_next(BufIFFT* unit, int inNumSamples);
void BufIFFT_Dtor(BufIFFT* unit);

void BufIFFT2_Ctor(BufIFFT2* unit);
void BufIFFT2_next(BufIFFT2* unit, int inNumSamples);
void BufIFFT2_Dtor(BufIFFT2* unit);

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

static int BufFFTBase_Ctor(BufFFTBase* unit, int framesize, int bufinput) {
    World* world = unit->mWorld;

    uint32 bufnum = (uint32)ZIN0(bufinput);
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
    //int framesize = (int)ZIN0(frmsizinput);
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


    if (!BufFFTBase_Ctor(unit, ZIN0(2), 0)) {
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

    //do nothing if the buffer number is less than 0
    if (fbufnum < 0.f) {
        ZOUT0(0) = -1.f;
        //Print("%f \n", -1.f);
        return;
    }

    memcpy(out, unit->m_fftsndbuf->data, unit->m_fftsndbuf->frames * sizeof(float));

    scfft_dofft(unit->m_scfft);
    unit->m_fftsndbuf->coord = coord_Complex;

    ZOUT0(0) = fbufnum;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufIFFT_Ctor(BufIFFT* unit) {
    int winType = sc_clip((int)ZIN0(1), -1, 1); // wintype may be used by the base ctor
    unit->m_wintype = winType;
    // These zeroes are to prevent the dtor freeing things that don't exist:
    unit->m_scfft = nullptr;

    if (!BufFFTBase_Ctor(unit, ZIN0(2), 0)) {
        SETCALC(*ClearUnitOutputs);
        return;
    }

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
    // if (unit->m_olabuf)
    //     RTFree(unit->mWorld, unit->m_olabuf);

    SCWorld_Allocator alloc(ft, unit->mWorld);
    if (unit->m_scfft)
        scfft_destroy(unit->m_scfft, alloc);
}

void BufIFFT_next(BufIFFT* unit, int wrongNumSamples) {
    float* out = OUT(0); // NB not ZOUT0

    // Load state from struct into local scope
    int pos = unit->m_pos;
    int audiosize = unit->m_audiosize;

    float fbufnum = ZIN0(0);

    int numSamples = unit->m_numSamples;

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
    float mult = ZIN0(4);

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

            chainBuf->data[i] = cubicinterp(fracphase, sourceBuf->data[bp0], sourceBuf->data[bp1], sourceBuf->data[bp2], sourceBuf->data[bp3])*mult; 
        
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


/////////////////////////////////////////////////////////////////////////////////////////////

//this constructor is needed because we need data structure that contains two fft buffers
static int BufFFTBase2_Ctor(BufFFTBase2* unit) {
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

    uint32 bufnum2 = (uint32)ZIN0(1);
    SndBuf* buf2;
    if (bufnum2 >= world->mNumSndBufs) {
        int localBufNum = bufnum2 - world->mNumSndBufs;
        Graph* parent = unit->mParent;
        if (localBufNum <= parent->localMaxBufNum) {
            buf2 = parent->mLocalSndBufs + localBufNum;
        } else {
            if (unit->mWorld->mVerbosity > -1) {
                Print("FFTBase_Ctor error: invalid buffer number: %i.\n", bufnum2);
            }
            return 0;
        }
    } else {
        buf2 = world->mSndBufs + bufnum2;
    }

    if (!buf2->data) {
        if (unit->mWorld->mVerbosity > -1) {
            Print("FFTBase_Ctor error: Buffer %i not initialised.\n", bufnum2);
        }
        return 0;
    }

    unit->m_fftsndbuf = buf;
    unit->m_fftbufnum = bufnum;
    unit->m_fullbufsize = buf->samples;

    unit->m_fftsndbuf2 = buf2;
    unit->m_fftbufnum2 = bufnum2;

    //framesize other than full buffer wouldn't make sense
    unit->m_audiosize = buf->samples;

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

    if (buf2->samples!=buf->samples) {
        Print("FFTBase_Ctor error: buffer sizes not equal.\n");
        return 0;
    } 

    unit->m_pos = 0;

    //ZOUT0(0) = ZIN0(0);

    return 1;
}

void BufIFFT2_Ctor(BufIFFT2* unit) {
    int winType = -1; //rectangular window
    
    unit->m_wintype = winType;
    // These zeroes are to prevent the dtor freeing things that don't exist:
    unit->m_olabuf = nullptr;
    unit->m_scfft = nullptr;

    if (!BufFFTBase2_Ctor(unit)) {
        SETCALC(*ClearUnitOutputs);
        return;
    }

    // This will hold the transformed and progressively overlap-added data ready for outputting.
    unit->m_olabuf = (float*)RTAlloc(unit->mWorld, unit->m_audiosize * sizeof(float));
    ClearUnitIfMemFailed(unit->m_olabuf);
    memset(unit->m_olabuf, 0, unit->m_audiosize * sizeof(float));

    //the ness window is a window that changes shape based on the correlation between frames
    //the ness window is half a frame - we use the first half of the window forwards and backwards
    unit->m_winbuf = (float*)RTAlloc(unit->mWorld, unit->m_audiosize/2 * sizeof(float));
    ClearUnitIfMemFailed(unit->m_winbuf);
    memset(unit->m_winbuf, 0, unit->m_audiosize/2 * sizeof(float));


    //allocate the two ifft instances
    SCWorld_Allocator alloc(ft, unit->mWorld);
    unit->m_scfft = scfft_create(unit->m_fullbufsize, unit->m_audiosize, (SCFFT_WindowFunction)unit->m_wintype,
                                 unit->m_fftsndbuf->data, unit->m_fftsndbuf->data, kBackward, alloc);
    ClearUnitIfMemFailed(unit->m_scfft);

    unit->m_scfft2 = scfft_create(unit->m_fullbufsize, unit->m_audiosize, (SCFFT_WindowFunction)unit->m_wintype,
                                 unit->m_fftsndbuf2->data, unit->m_fftsndbuf2->data, kBackward, alloc);
    ClearUnitIfMemFailed(unit->m_scfft2);


    // "pos" will be reset to zero when each frame comes in. Until then, the following ensures silent output at first:
    unit->m_pos = 0; // unit->m_audiosize;

    if (unit->mCalcRate == calc_FullRate) {
        unit->m_numSamples = unit->mWorld->mFullRate.mBufLength;
    } else {
        unit->m_numSamples = 1;
    }

    SETCALC(BufIFFT2_next);
    ClearUnitOutputs(unit, 1);
}

void BufIFFT2_Dtor(BufIFFT2* unit) {
    if (unit->m_olabuf)
        RTFree(unit->mWorld, unit->m_olabuf);

    if (unit->m_winbuf)
        RTFree(unit->mWorld, unit->m_winbuf);

    //destroy both scfft instances
    SCWorld_Allocator alloc(ft, unit->mWorld);
    if (unit->m_scfft)
        scfft_destroy(unit->m_scfft, alloc);
    if (unit->m_scfft2)
        scfft_destroy(unit->m_scfft2, alloc);
}

void BufIFFT2_next(BufIFFT2* unit, int wrongNumSamples) {
    float* out = OUT(0); // NB not ZOUT0

    // Load state from struct into local scope
    int pos = unit->m_pos;
    int audiosize = unit->m_audiosize;

    int numSamples = unit->m_numSamples;
    float* olabuf = unit->m_olabuf;

    float fbufnum1 = ZIN0(0); 
    float fbufnum2 = ZIN0(1); 

    //do the IFFT if either of these buffers is positive
    if (fbufnum1 >=0.f || fbufnum2 >=0.f) {
        float* fftbuf;

        if(fbufnum1>=0.f){
            // Ensure it's in cartesian format, not polar
            ToComplexApx(unit->m_fftsndbuf);

            //point the ifft object to the incoming buffer

            fftbuf = unit->m_fftsndbuf->data;

            scfft_doifft(unit->m_scfft);
        } else {
            // Ensure it's in cartesian format, not polar
            ToComplexApx(unit->m_fftsndbuf2);

            //point the ifft object to the incoming buffer

            fftbuf = unit->m_fftsndbuf2->data;

            scfft_doifft(unit->m_scfft2);
        }

        int hopsamps = audiosize/2;

        // This UGen should only be used with a hopsize of 0.5, so we move the second half of the olabuf to the front
        memmove(olabuf, olabuf + hopsamps, hopsamps * sizeof(float));

        //olabuf is shifted so calculate the correlation between the beginning of each buffer


        float correlation = calculate_correlation(olabuf, fftbuf, hopsamps);
        
        //make the correlation-based overlap window
        make_ness_window(unit->m_winbuf, audiosize, correlation);

        //multiply the start of the fftbuf and the shifted olabuf by the nesswindow

        //would be great to replace this with vDSP_vmul
        #if defined(__APPLE__) && !defined(SC_IPHONE)
            vDSP_vmul(fftbuf, 1, unit->m_winbuf, 1, fftbuf, 1, hopsamps);
            vDSP_vrvrs(unit->m_winbuf, 1, hopsamps);
            vDSP_vmul(olabuf, 1, unit->m_winbuf, 1, olabuf, 1, hopsamps);
        #else
        for (pos = 0; pos<hopsamps; pos++) {
            fftbuf[pos] = fftbuf[pos]*unit->m_winbuf[pos];
            olabuf[pos] = olabuf[pos]*unit->m_winbuf[hopsamps-1-pos];
        }
        #endif
// Then mix the "new" time-domain data in - adding at first, then just setting (copying) where the "old" is supposed to
// be zero.
        #if defined(__APPLE__) && !defined(SC_IPHONE)
                vDSP_vadd(olabuf, 1, fftbuf, 1, olabuf, 1, hopsamps);
        #else
        // NB we re-use the "pos" variable temporarily here for write rather than read
        for (pos = 0; pos < hopsamps; ++pos) {
            olabuf[pos] += fftbuf[pos];  
        }
        #endif
        memcpy(olabuf + hopsamps, fftbuf + hopsamps, (hopsamps) * sizeof(float));

        // Move the pointer back to zero, which is where playback will next begin
        pos = 0;

    } // End of has-the-chain-fired

    // Now we can output some stuff, as long as there is still data waiting to be output.
    // If there is NOT data waiting to be output, we output zero. (Either irregular/negative-overlap
    //     FFT firing, or FFT has given up, or at very start of execution.)
    if (pos >= audiosize)
        ClearUnitOutputs(unit, numSamples);
    else {
        memcpy(out, olabuf + pos, numSamples * sizeof(float));
        pos += numSamples;
    }
    unit->m_pos = pos;
}


void PV_AccumPhase_next(PV_Unit* unit, int inNumSamples) {
    PV_GET_BUF2

    SCPolarBuf* p = ToPolarApx(buf1);
    SCPolarBuf* q = ToPolarApx(buf2);

    //zero the dc and nyquist components?
    p->dc = 0.f;
    p->nyq = 0.f;

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
    DefineDtorUnit(BufIFFT2);
    DefinePVUnit(BufFFT_BufCopy);
    DefinePVUnit(PV_AccumPhase);
    DefineSimpleUnit(BufFFTTrigger);
    DefineSimpleUnit(BufFFTTrigger2);
}



