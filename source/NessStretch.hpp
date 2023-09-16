// NessStretch.hpp
// NessStretch (spluta@gmail.com)

#pragma once

#include "SC_PlugIn.hpp"
#include "ness_stretch/ness_stretch.h"

namespace NessStretchUGen {

class NessStretchUGen : public SCUnit {
public:
    NessStretchUGen();

    // Destructor
    ~NessStretchUGen();

private:
    // Calc function
    void next(int nSamples);
    float m_freqMul{2.0f/(float)sampleRate()};
    const float buf1In {in0(0)};
    const float dur_mult {in0(1)};
    const int num_slices {(int)in0(2)};
    //const int extreme {(int)in0(4)};
    const int extreme {(int)sc_clip(in0(3), 0, 10)};
    const int paul_stretch_win_size {(int)sc_clip(in0(4), 1,3)};
    //const int max_win_size {((int)round(sampleRate()/44100.f))*((int)pow(2.0, 7+num_slices))}; //65536 if 44.1 or 48k, *2 for every multiple

    //usize::pow(2, 7+num_slices as u32) * (sample_rate as usize/44100);
    const int max_win_size {((int)round(sampleRate()/44100.f))*65536}; //65536 if 44.1 or 48k, *2 for every multiple - (int)pow(2, 7+num_slices
    //const int actual_max_win_size {max_win_size};//{(int)pow(2, 7+num_slices)};
    float* outStorageBuf;
    //float outStorageBuf[65536*4] = {}; //this is a hard-coded max
    //std::vector<float> outStorageBuf = std::vector<float>(max_win_size);
    int m_waitCounter {0};
    int m_offset {0};

    NessStretchR *rusty;
};

} // namespace NessStretch

