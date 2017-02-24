
cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fpermissive -std=c++11 -D__STDC_CONSTANT_MACROS -DFIXED_POINT -DUSE_KISS_FFT -DEXPORT="" -UHAVE_CONFIG_H ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2 -DSYS=posix -DNO_CRYPTO")

set(main_path ${CMAKE_SOURCE_DIR}/src/main/cpp)

file(GLOB cpp_path ${main_path}/*.cpp)
file(GLOB rtmp_path ${main_path}/librtmp/*.c)

find_library(log-lib log )

#
add_library(x264 STATIC IMPORTED)
set_target_properties(x264 PROPERTIES IMPORTED_LOCATION ${main_path}/lib/libx264.a)

#
add_library(faac STATIC IMPORTED)
set_target_properties(faac PROPERTIES IMPORTED_LOCATION ${main_path}/lib/libfaac.a)

#
add_library(rtmp-lib STATIC ${rtmp_path})

#
add_library(native-lib SHARED ${cpp_path})
add_library(que-lib STATIC src/main/cpp/queue.c src/main/cpp/RtmpPush.cpp)
target_link_libraries(que-lib)

target_link_libraries(native-lib que-lib rtmp-lib x264 faac ${log-lib} )

include_directories(${main_path})