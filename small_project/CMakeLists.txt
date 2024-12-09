cmake_minimum_required(VERSION 3.5.0)
project(QTalk VERSION 0.1.0 LANGUAGES C CXX)

add_executable(QTalk 
main.cpp
ChatServer.cpp)

set(EXECUTABLE_OUTPUT_PATH ../)

target_link_libraries(QTalk
    muduo_net
    muduo_base
    pthread
)

