cmake_minimum_required(VERSION 3.5.0)
project(mymuduo)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

aux_source_directory(. SRC_LIST)

add_library(mymuduo SHARED ${SRC_LIST})
