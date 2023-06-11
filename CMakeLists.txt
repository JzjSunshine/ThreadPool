cmake_minimum_required(VERSION 3.16)
project(threadPool)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

add_executable(threadPool main.cpp include/taskqueue.h include/threadpool.h src/threadpool.cpp src/taskqueue.cpp)

target_link_libraries(threadPool pthread)
