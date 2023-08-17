BufFFT Library
Sam Pluta
 

The BufFFT Library augment's SuperCollider's FFT libraries by enabling FFT operations on buffers rather than on audio streams only. This allows time stretching operations and operations on non chrono-linear sets of grains, two processes that are difficult and inefficient with the standard FFT implementaton. Luckily, all standard SuperCollider PhaseVocoder PV operations should still function normally with BufFFT.

run the following from this directory to build from source using cmake

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DSC_PATH=<PATH TO SC SOURCE> 
cmake --build . --config Release
```

It should build BufFFTTrigger, BufFFTTrigger2, BufFFT_BufCopy, BufFFT, BufIFFT, and PV_AccumPhase plugins.
# BufFFT
