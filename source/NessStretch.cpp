// PluginNessStretch.cpp
// Rust Saw (spluta@gmail.com)

#include "SC_PlugIn.hpp"
#include "NessStretch.hpp"
#include "ness_stretch/ness_stretch.h"
#include <thread>

static InterfaceTable* ft;

extern "C" void show_vector(float const *data, std::size_t size);

namespace NessStretchUGen {

NessStretchUGen::NessStretchUGen() {
    //outStorageBuf = (float*)RTAlloc(mWorld, max_win_size* sizeof(float));
    //ClearUnitIfMemFailed(outStorageBuf);
    //memset(outStorageBuf, 1, max_win_size * sizeof(float));

    //Print("%i %i %i \n", max_win_size, num_slices, extreme);

    int win_size_divisor = 1;
    // if (num_slices==8) {win_size_divisor = 2;}
    // if (num_slices==7) {win_size_divisor = 4;}
    // if (num_slices==6) {win_size_divisor = 8;}
    // if (num_slices<=5) {win_size_divisor = 16;}

    // Print("win_size_divisor %i \n", win_size_divisor);

    rusty = ness_stretch_new(dur_mult, max_win_size, win_size_divisor, num_slices, extreme, paul_stretch_win_size);

    mCalcFunc = make_calc_function<NessStretchUGen, &NessStretchUGen::next>();
}

NessStretchUGen::~NessStretchUGen() {
    ness_stretch_free(rusty);
}

void NessStretchUGen::next(int nSamples) {
    float* outBuf = out(0);

    float fbufnum1 = in0(0); //inBuf

    //do nothing if the buffer number is less than 0
    if (fbufnum1 >= 0.f) {

        uint32 ibufnum1 = (int)fbufnum1;
        World* world = mWorld;
        SndBuf* inBuf;
        if (ibufnum1 >= world->mNumSndBufs) {
            int localBufNum = ibufnum1 - world->mNumSndBufs;
            Graph* parent = mParent;
            if (localBufNum <= parent->localBufNum) {
                inBuf = parent->mLocalSndBufs + localBufNum;
            } else {
                inBuf = world->mSndBufs;
            }
        } else {
            inBuf = world->mSndBufs + ibufnum1;
        }

        //float* outStorageTemp = (float*)RTAlloc(mWorld, max_win_size* sizeof(float));

        //this would be more efficient if I could send the whole array of data over at once
        //for some reason, I can't get this to work with arrays I create (works with input or output buffers, but not sound buffers)
        for (int i = 0; i<inBuf->samples; i++)
        {
            ness_stretch_set_in_chunk(rusty, inBuf->data[i], i);
        }
        //do the ness_stretch
        ness_stretch_calc(rusty);
        ness_stretch_move_in_to_stored(rusty);
        //std::thread nessy(ness_stretch_calc, rusty, actual_max_win_size);

        m_offset = 0;

        //m_waitCounter = 10;
        //Print("%i %i calc \n", m_waitCounter, m_offset);

        RELEASE_SNDBUF_SHARED(inBuf);
    }

    // if (m_waitCounter>0) {
    //     m_waitCounter -= 1;
    // } else {
    //     if (m_waitCounter == 0){
    //         //Print("%i %i \n", m_waitCounter, m_offset);
    //         m_offset = 0;
    //         m_waitCounter = -1;
            
    //         ness_stretch_move_in_to_stored(rusty);
    //     }
    // }

    if (m_offset>=0){
        ness_stretch_next(rusty, outBuf, nSamples, m_offset);
    } else {
        for (int i = 0; i<nSamples; i++)
            outBuf[i]=0.f;
    }

    

    m_offset += nSamples;
}

} // namespace NessStretchUGen


PluginLoad(NessStretchUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<NessStretchUGen::NessStretchUGen>(ft, "NessStretchUGen", false);
}
