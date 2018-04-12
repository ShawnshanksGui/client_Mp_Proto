cmake_minimum_required(VERSION 2.8)

project (test)

add_compile_options(-std=c++11)

#for debugging
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV {CXXFLAGS} -O3 -wall")
#

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#aux_source_directory(video_reader.cpp data_manager.cpp codeStreaming_parser.cpp #bitrate_select.cpp fec_param_adjustor.cpp utility.cpp DIR_SRCS)

add_subdirectory(rs_fft)

#add_executable(test_writer DIR_SRCS)
add_executable(test main.cpp video_writer.cpp data_manager.cpp mySocket.cpp decoder.cpp timer.cpp   myUtility.cpp thread_core_affinity_set.cpp)

#link to static library
#target_link_libraries(test_writer Threads::Threads)
target_link_libraries(test ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(test RS_FFT)
