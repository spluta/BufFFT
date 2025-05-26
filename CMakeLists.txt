cmake_minimum_required (VERSION 2.8)
project (BufFFT)
set(RUST_DIR "source/ness_stretch")

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

set(CMAKE_CXX_STANDARD 17)


set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(BufFFT MODULE source/BufFFT.cpp)
add_library(BufIFFT MODULE source/BufFFT.cpp)
add_library(BufIFFT2 MODULE source/BufFFT.cpp)
add_library(BufFFTTrigger MODULE source/BufFFT.cpp)
add_library(BufFFTTrigger2 MODULE source/BufFFT.cpp)
add_library(BufFFT_BufCopy MODULE source/BufFFT.cpp)
add_library(PV_AccumPhase MODULE source/BufFFT.cpp)
add_library(NessStretchUGen MODULE source/NessStretch.cpp)

if (APPLE)
	target_link_libraries(BufFFT "-framework Accelerate")
    target_link_libraries(BufIFFT "-framework Accelerate")
    target_link_libraries(BufIFFT2 "-framework Accelerate")
    target_link_libraries(BufFFTTrigger "-framework Accelerate")
    target_link_libraries(BufFFTTrigger2 "-framework Accelerate")
    target_link_libraries(BufFFT_BufCopy "-framework Accelerate")
    target_link_libraries(PV_AccumPhase "-framework Accelerate")
endif()

# we need all this stuff to inculde the rust lib file
include_directories ("${PROJECT_BINARY_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}${RUST_DIR}")

include (ExternalProject)
set_directory_properties (PROPERTIES EP_PREFIX ${PROJECT_BINARY_DIR}/RUST)

# add_dependencies(NessStretchUGen ${RUST_DIR})

# Since static libraries can't communicate their own dependencies to the linker, we need to
# specify them explicitly, AFTER the static libraries (here, pthread and dl)
# We can ask Cargo for a list of the required dependencies by using the command
# `cargo rustc -- --print native-static-libs`, or by examining the error messages when
# using the `make` command.
TARGET_LINK_LIBRARIES(
  NessStretchUGen
  debug "${PROJECT_SOURCE_DIR}/${RUST_DIR}/target/debug/lib${RUST_DIR}.a"
  optimized "${PROJECT_SOURCE_DIR}/${RUST_DIR}/target/release/libness_stretch.a"
  dl
  pthread
)

# set installation directory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${lib_name}" CACHE PATH "Install prefix" FORCE)
else()
  set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/${lib_name}")
endif()

install(DIRECTORY "${lib_name}/Classes" "${lib_name}/HelpSource" DESTINATION "${CMAKE_INSTALL_PREFIX}")

install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX}/scx_files")

if(APPLE)
  install(FILES ${CMAKE_BINARY_DIR}/BufFFT.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/BufIFFT.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  
  install(FILES ${CMAKE_BINARY_DIR}/BufIFFT2.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/BufFFTTrigger.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/BufFFTTrigger2.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/BufFFT_BufCopy.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/PV_AccumPhase.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/NessStretchUGen.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
endif()

if(WIN32)
install(FILES ${CMAKE_BINARY_DIR}/Release/BufFFT.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/Release/BufIFFT.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  
  install(FILES ${CMAKE_BINARY_DIR}/Release/BufIFFT2.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/Release/BufFFTTrigger.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/Release/BufFFTTrigger2.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/Release/BufFFT_BufCopy.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/Release/PV_AccumPhase.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)
  install(FILES ${CMAKE_BINARY_DIR}/Release/NessStretchUGen.scx DESTINATION ${CMAKE_INSTALL_PREFIX}/scx_files)

endif()
