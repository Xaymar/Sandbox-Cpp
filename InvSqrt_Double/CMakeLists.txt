cmake_minimum_required(VERSION 2.8)
project(InvSqrtDouble)

set(SOURCES 
    InvSqrt_Double.cpp)

set(HEADERS)

add_executable(InvSqrtDouble 
    ${SOURCES} 
    ${HEADERS})

SET(PLATFORM_LIBS)
if(WIN32)
	SET(PLATFORM_LIBS
		winmm)
endif()

target_link_libraries(InvSqrtDouble
	${PLATFORM_LIBS})