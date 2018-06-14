cmake_minimum_required(VERSION 2.8)

project(hackrf_recode)

include_directories("/usr/local/include/libhackrf")

add_executable(demo test.cpp )

target_link_libraries(demo libhackrf.so)


