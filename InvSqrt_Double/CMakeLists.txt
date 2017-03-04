cmake_minimum_required(VERSION 2.8)
project(InvSqrtDouble)

set(SOURCES 
    InvSqrt_Double.cpp)

set(HEADERS)

add_executable(InvSqrtDouble 
    ${SOURCES} 
    ${HEADERS})

target_link_libraries(InvSqrtDouble)