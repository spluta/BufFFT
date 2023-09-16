# BufFFT Library

Sam Pluta


The BufFFT Library augment's SuperCollider's FFT libraries by enabling FFT operations on buffers rather than on audio streams only. This allows time stretching operations and operations on non chrono-linear sets of grains, two processes that are difficult and inefficient with the standard FFT implementaton. Luckily, all standard SuperCollider PhaseVocoder PV operations should still function normally with BufFFT.


### NessStretch time stretching algorithm as a SuperCollider plugin!

In addition to standard Buffer FFT processes, this library also includes the NessStretch time stretching algorithm. NessStretch implements a phase randomized real fft (using rustfft) time stretch algorithm, the NessStretch, which splits the original sound file into 9 discrete frequency bands, and uses a decreasing frame size to correspond to increasing frequency. Starting with a largest frame of 65536, the algorithm will use the following frequency band/frame size breakdown (assuming 44100 Hz input):

0-86 Hz : 65536 frames, 86-172 : 32768, 172-344 : 16384, 344-689 : 8192, 689-1378 : 4096, 1378-2756 : 2048, 2756-5512 : 1024, 5512-11025 : 512, 11025-22050 : 256.

The NessStretch is a refinement of Paul Nasca's excellent PaulStretch algorithm. PaulStretch uses a single frame size throughout the entire frequency range. The NessStretch's layered analysis bands are a better match for human frequency perception, and do a better job of resolving shorter, noisier high-frequency sounds (sibilance, snares, etc.). A convenience class for PaulStretch is included in this repository.

### Building and Installation

In order to build this project, you will need both CMake and Cargo installed, as there are elements written in both C++ and Rust. You will also need the SuperCollider source code downloaded.

If you only want the BufFFT plugins, you can skip the Rust cargo build, but then the NessStretchUGen will fail to build.

## Compiling the Rust ness_stretch library

You will need Rust cargo to compile the ness_stretch Rust library. Rust is super easy to install. Try "brew install rust" or find directions here: https://www.rust-lang.org/tools/install

In the terminal, from the project directory "BufFFT, change directory into the "BufFFT/source/ness_stretch" directory:

```
cd source/ness_stretch
```
 and build the rust code for this project with cargo:
```
cargo build --release
```
return to the project root directory:
```
cd ../..
```

then in the project root directory "BufFFT", run:

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DSC_PATH="<Path to SC Source>"
cmake --build . --config Release
```

as you normally would when building SC plugins.

It should build BufFFTTrigger, BufFFTTrigger2, BufFFT_BufCopy, BufFFT, BufIFFT, BufIFFT2, PV_AccumPhase, and NessStretchUGen plugins. The .scx files from the "build" directory, the HelpSource directory, and the Classes directory all need to be the SC Extensions path (I simply place the entire BufFFT directory in the Extensions folder) and recompile the class library. The code in the help file should run.
