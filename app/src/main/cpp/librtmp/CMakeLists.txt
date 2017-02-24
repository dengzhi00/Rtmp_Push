#版本声明
cmake_minimum_required(VERSION 3.4.1)

file(GLOB my_cpp ${CMAKE_SOURCE_DIR}/src/main/cpp/librtmp/*.c)
MESSAGE("我的rtmp源文件:${my_cpp}")

set(CMAKE_CXX_GLAGS "${CMAKE_CXX_FLAGS} -Wall -O2 -DSYS=posix -DNO_CRYPTO")