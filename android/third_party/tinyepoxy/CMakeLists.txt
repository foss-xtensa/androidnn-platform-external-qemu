cmake_minimum_required(VERSION 3.5)

android_add_library(
  TARGET emulator-tinyepoxy LICENSE Apache-2.0 SRC # cmake-format: sortable
                                                   tinyepoxy.cpp)

target_include_directories(emulator-tinyepoxy
                           PRIVATE ../../android-emugl/host/include)
