cmake_minimum_required(VERSION 2.8)
project(InvSqrtSingle)

set(SOURCES 
    InvSqrt_Single.cpp)

set(HEADERS)

add_executable(InvSqrtSingle 
    ${SOURCES} 
    ${HEADERS})

SET(PLATFORM_LIBS)
if(WIN32)
	SET(PLATFORM_LIBS
		winmm)
endif()

target_link_libraries(InvSqrtSingle
	${PLATFORM_LIBS})