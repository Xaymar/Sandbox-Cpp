cmake_minimum_required(VERSION 2.8)
project(InvSqrtSingle)

set(SOURCES 
    InvSqrt_Single.cpp)

set(HEADERS)

add_executable(advmemcpy 
    ${SOURCES} 
    ${HEADERS})

target_link_libraries(advmemcpy)