
// void BufIFFT2_next(BufIFFT2* unit, int wrongNumSamples) {
//     float* out = OUT(0); // NB not ZOUT0

//     // Load state from struct into local scope
//     int pos = unit->m_pos;
//     int audiosizeDiv2 = unit->m_audiosize/2;

//     //float* olabuf = unit->m_olabuf;
//     float fbufnum1 = ZIN0(0);
//     float fbufnum2 = ZIN0(1);

//     int numSamples = unit->m_numSamples;

//     Print("%f %f \n", fbufnum1, fbufnum2);

//     // // Only run the BufIFFT if we're receiving a new block of input data - otherwise just output data already received
//     // if (fbufnum1 >= 0.f) {
//     //     // Ensure it's in cartesian format, not polar
//     //     ToComplexApx(unit->m_fftsndbuf);
//     //     scfft_doifft(unit->m_scfft);


//     //     //make the nesswindow
//     //     float correlation = calculate_correlation(unit->m_outbuf+audiosizeDiv2, unit->m_fftsndbuf->data, audiosizeDiv2);
        
//     //     Print("%f \n", correlation);

//     //     make_ness_window(unit->m_winbuf, unit->m_audiosize, correlation);

//     //     //crossfade end of outbuf with beginning of m_fftsndbuf->data
//     //     //and place it at the beginning of the buffer
//     //     for (int i = 0; i<audiosizeDiv2; i++) {
//     //         unit->m_outbuf[i] = unit->m_outbuf[i+audiosizeDiv2]*unit->m_winbuf[i] + unit->m_fftsndbuf->data[i]*unit->m_winbuf[i+audiosizeDiv2];
//     //         unit->m_outbuf[i+audiosizeDiv2] = unit->m_fftsndbuf->data[i+audiosizeDiv2];
//     //     }
//     //     //move end of outbuf to top of outbuf
//     //     //memcpy(unit->m_outbuf, unit->m_outbuf+audiosizeDiv2, audiosizeDiv2 * sizeof(float));
//     //     // Move the pointer back to zero, which is where playback will next begin
//     //     pos = 0;

//     // }
   

//     // if (fbufnum2 >= 0.f) {
//     //     // Ensure it's in cartesian format, not polar
//     //     ToComplexApx(unit->unit2->m_fftsndbuf);
//     //     scfft_doifft(unit->unit2->m_scfft);


//     //     //make the nesswindow
//     //     float correlation = calculate_correlation(unit->m_outbuf+audiosizeDiv2, unit->unit2->m_fftsndbuf->data, audiosizeDiv2);
        
//     //     make_ness_window(unit->m_winbuf, unit->m_audiosize, correlation);

//     //     //crossfade end of outbuf with beginning of m_fftsndbuf->data
//     //     //and place it at the beginning of the buffer
//     //     for (int i = 0; i<audiosizeDiv2; i++) {
//     //         unit->m_outbuf[i] = 
//     //             unit->m_outbuf[i+audiosizeDiv2]*unit->m_winbuf[i] + 
//     //             unit->unit2->m_fftsndbuf->data[i]*unit->m_winbuf[i+audiosizeDiv2];
//     //         unit->m_outbuf[i+audiosizeDiv2] = unit->unit2->m_fftsndbuf->data[i+audiosizeDiv2];
//     //     }


//     //     // Move the pointer back to zero, which is where playback will next begin
//     //     pos = 0;

//     // }

//     // if (pos >= (audiosizeDiv2))
//     //     ClearUnitOutputs(unit, numSamples);
//     // else {
//     //     memcpy(out, unit->m_outbuf+pos, numSamples * sizeof(float));
//     //     pos += numSamples;
//     // }
    
//     for (int i = 0; i<numSamples; i++)
//         out[i]= 0.f;

//     unit->m_pos = pos;
// }
// void BufIFFT2_Ctor(BufIFFT2* unit) {
//     //int winType = sc_clip((int)ZIN0(1), -1, 1); // wintype may be used by the base ctor
//     //unit->m_wintype = winType;
//     // These zeroes are to prevent the dtor freeing things that don't exist:
//     unit->m_winbuf = nullptr;
//     unit->m_scfft = nullptr;

//     if (!BufFFTBase_Ctor(unit, 3, 0)) {
//         SETCALC(*ClearUnitOutputs);
//         return;
//     }

//     // if (!BufFFTBase_Ctor(unit->unit2, 3, 1)) {
//     //     SETCALC(*ClearUnitOutputs);
//     //     return;
//     // }

//     // Stores the crossfade window
//     unit->m_winbuf = (float*)RTAlloc(unit->mWorld, unit->m_audiosize * sizeof(float));
//     ClearUnitIfMemFailed(unit->m_winbuf);
//     memset(unit->m_winbuf, 0, unit->m_audiosize * sizeof(float));


//     // Stores the output buffer
//     unit->m_outbuf = (float*)RTAlloc(unit->mWorld, unit->m_audiosize * sizeof(float));
//     ClearUnitIfMemFailed(unit->m_winbuf);
//     memset(unit->m_outbuf, 0, unit->m_audiosize * sizeof(float));


//     SCWorld_Allocator alloc(ft, unit->mWorld);
//     unit->m_scfft = scfft_create(unit->m_fullbufsize, unit->m_audiosize, (SCFFT_WindowFunction)(-1),
//                                  unit->m_fftsndbuf->data, unit->m_fftsndbuf->data, kBackward, alloc);
//     ClearUnitIfMemFailed(unit->m_scfft);

//     // //process second unit
//     // SCWorld_Allocator alloc2(ft, unit->unit2->mWorld);
//     // unit->unit2->m_scfft = scfft_create(unit->unit2->m_fullbufsize, unit->unit2->m_audiosize, (SCFFT_WindowFunction)(-1),
//     //                              unit->unit2->m_fftsndbuf->data, unit->unit2->m_fftsndbuf->data, kBackward, alloc);
//     // ClearUnitIfMemFailed(unit->unit2->m_scfft);


//     // "pos" will be reset to zero when each frame comes in. Until then, the following ensures silent output at first:
//     unit->m_pos = 0; // unit->m_audiosize;


    

//     if (unit->mCalcRate == calc_FullRate) {
//         unit->m_numSamples = unit->mWorld->mFullRate.mBufLength;
//     } else {
//         unit->m_numSamples = 1;
//     }

//     // if (unit->unit2->mCalcRate == calc_FullRate) {
//     //     unit->unit2->m_numSamples = unit->unit2->mWorld->mFullRate.mBufLength;
//     // } else {
//     //     unit->unit2->m_numSamples = 1;
//     // }

//     SETCALC(BufIFFT2_next);
//     ClearUnitOutputs(unit, 1);
//     //ClearUnitOutputs(unit->unit2, 1);
// }

// void BufIFFT2_Dtor(BufIFFT2* unit) {
//     if (unit->m_winbuf)
//         RTFree(unit->mWorld, unit->m_winbuf);
//     if (unit->m_outbuf)
//         RTFree(unit->mWorld, unit->m_winbuf);

//     SCWorld_Allocator alloc(ft, unit->mWorld);
//     if (unit->m_scfft)
//         scfft_destroy(unit->m_scfft, alloc);
//     SCWorld_Allocator alloc2(ft, unit->unit2->mWorld);
//     if (unit->m_scfft)
//         scfft_destroy(unit->unit2->m_scfft, alloc2);
// }


// void BufIFFT2_Ctor(BufIFFT2* unit) {
//     int winType = sc_clip((int)ZIN0(1), -1, 1); // wintype may be used by the base ctor
//     unit->m_wintype = winType;
//     // These zeroes are to prevent the dtor freeing things that don't exist:
//     unit->m_scfft = nullptr;

//     if (!BufFFTBase_Ctor(unit, ZIN0(2), 0)) {
//         SETCALC(FFT_ClearUnitOutputs);
//         return;
//     }

//     SCWorld_Allocator alloc(ft, unit->mWorld);
//     unit->m_scfft = scfft_create(unit->m_fullbufsize, unit->m_audiosize, (SCFFT_WindowFunction)unit->m_wintype,
//                                  unit->m_fftsndbuf->data, unit->m_fftsndbuf->data, kBackward, alloc);
//     ClearUnitIfMemFailed(unit->m_scfft);

//     // "pos" will be reset to zero when each frame comes in. Until then, the following ensures silent output at first:
//     unit->m_pos = 0; // unit->m_audiosize;

//     if (unit->mCalcRate == calc_FullRate) {
//         unit->m_numSamples = unit->mWorld->mFullRate.mBufLength;
//     } else {
//         unit->m_numSamples = 1;
//     }

//     SETCALC(BufIFFT2_next);

//     //ClearUnitOutputs(unit, 1);
// }

// void BufIFFT2_Dtor(BufIFFT2* unit) {
//     SCWorld_Allocator alloc(ft, unit->mWorld);
//     if (unit->m_scfft)
//         scfft_destroy(unit->m_scfft, alloc);
// }

// void BufIFFT2_next(BufIFFT2* unit, int wrongNumSamples) {
    

//     // Load state from struct into local scope
//     int pos = unit->m_pos;
//     int audiosize = unit->m_audiosize;

//     float fbufnum = ZIN0(0);
//     ZOUT0(0) = -1.f;

//     int numSamples = unit->m_numSamples;

//     // Only run the BufIFFT if we're receiving a new block of input data - otherwise just output data already received
//     if (fbufnum >= 0.f) {
//         ZOUT0(0) = fbufnum;

//         // Ensure it's in cartesian format, not polar
//         ToComplexApx(unit->m_fftsndbuf);

//         scfft_doifft(unit->m_scfft);

//     } // End of has-the-chain-fired
   

// }





// void BufFFT_CrossFade_Ctor(BufFFT_CrossFade* unit) {
//     if (!BufFFTBase_Ctor(unit, 0, 0)) {
//         SETCALC(FFT_ClearUnitOutputs);
//         return;
//     }
//     int audiosize = unit->m_audiosize * sizeof(float);
//     unit->m_audiosizeDiv2 = unit->m_audiosize/2;

//     unit->m_pos = 0;

//     // Stores the crossfade window
//     unit->m_winbuf = (float *)RTAlloc(unit->mWorld, audiosize * sizeof(float));
//     ClearUnitIfMemFailed(unit->m_winbuf);
//     memset(unit->m_winbuf, 0, audiosize * sizeof(float));

//     // Stores the output buffer
//     unit->m_outbuf = (float *)RTAlloc(unit->mWorld, audiosize * sizeof(float));
//     ClearUnitIfMemFailed(unit->m_outbuf);
//     memset(unit->m_outbuf, 0, audiosize * sizeof(float));

//     Print("contstructor \n");

//     SETCALC(BufFFT_CrossFade_next);
// }

// void BufFFT_CrossFade_Dtor(BufFFT_CrossFade* unit) {
//     // SCWorld_Allocator alloc(ft, unit->mWorld);
//     // if (unit->m_scfft)
//     //     scfft_destroy(unit->m_scfft, alloc);

//     if (unit->m_winbuf)
//         RTFree(unit->mWorld, unit->m_winbuf);
//     if (unit->m_outbuf)
//         RTFree(unit->mWorld, unit->m_outbuf);
// }

// void BufFFT_CrossFade_next(BufFFT_CrossFade* unit, int inNumSamples) {
//     float fbufnum1 = ZIN0(0); 
//     float fbufnum2 = ZIN0(1); 
//     float* out = OUT(0); 

//     int pos = unit->m_pos;

//     //Print("%f %f \n", fbufnum1, fbufnum2);

//     int ibufnum = -1;

//     if (fbufnum1 >=0.f)
//         ibufnum = (int)fbufnum1;
//     else if (fbufnum2>=0.f)
//         ibufnum = (int)fbufnum2;

//     //do the crossfade if either of these buffers is positive
//     if (ibufnum >=0 ) {
//         Print("%i  \n", ibufnum);

//         // //get the buffer
//         uint32 ibufnum1 = ibufnum;
//         World* world = unit->mWorld;
//         SndBuf* buf;
//         if (ibufnum1 >= world->mNumSndBufs) {
//             int localBufNum = ibufnum1 - world->mNumSndBufs;
//             Graph* parent = unit->mParent;
//             if (localBufNum <= parent->localBufNum) {
//                 buf = parent->mLocalSndBufs + localBufNum;
//             } else {
//                 buf = world->mSndBufs;
//             }
//         } else {
//             buf = world->mSndBufs + ibufnum1;
//         }

//         //unit->m_fftsndbuf = unit->mWorld->mSndBufs + ibufnum1;

//         //make the nesswindow
//         // for (int i=0; i<unit->m_audiosize; i++)
//         //     Print("%f ", unit->m_outbuf[i]);
//         // Print("\n");
//         // for (int i=0; i<unit->m_audiosize; i++)
//         //     Print("%f ", buf->data[i]);
//         // Print("\n");

//         int audiosize = buf->samples;
//         int sizeDiv2 = audiosize/2;


//         float correlation = calculate_correlation(unit->m_outbuf, buf->data, sizeDiv2);
//         //Print("%i %i %f  \n", unit->m_audiosize, sizeDiv2, correlation);
        
//         make_ness_window(unit->m_winbuf, audiosize, correlation);

//         // for (int i=0; i<unit->m_audiosize; i++)
//         //    Print("%f ", unit->m_winbuf[i]);
//         // Print("\n");

//         //crossfade end of outbuf with beginning of m_fftsndbuf->data
//         //and place it at the beginning of the buffer
//         for (int i = 0; i<sizeDiv2; i++) {
//             unit->m_outbuf[i] = 
//                 unit->m_outbuf[i+sizeDiv2]*unit->m_winbuf[sizeDiv2-i-1] + 
//                 buf->data[i]*unit->m_winbuf[i];

//             unit->m_outbuf[i+sizeDiv2] = buf->data[i+sizeDiv2];
//         }

//         for (int i=0; i<20; i++)
//            Print("%f ", unit->m_outbuf[i]);
//         Print("\n");

//         pos = 0;
//         RELEASE_SNDBUF_SHARED(buf);
//     }
//     // for (int i = 0; i<inNumSamples; i++)
//     //      out[i]= 0.f;
    
    

//     //Print("%i ", pos);

//     if (pos >= (unit->m_audiosize/2))
//         ClearUnitOutputs(unit, inNumSamples);
//     else
//     {
//         //Print("play it");
//         memcpy(out, unit->m_outbuf + pos, inNumSamples * sizeof(float));
//         pos += inNumSamples;
//     }
//     unit->m_pos = pos;
// }
