cmake_minimum_required (VERSION 2.8)
project (OversamplingOscillators)

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(BufFFT MODULE BufFFT.cpp)
add_library(BufIFFT MODULE BufFFT.cpp)
add_library(BufFFTTrigger MODULE BufFFT.cpp)
add_library(BufFFTTrigger2 MODULE BufFFT.cpp)
add_library(BufFFT_BufCopy MODULE BufFFT.cpp)
add_library(PV_AccumPhase MODULE BufFFT.cpp)