cmake_minimum_required(VERSION 2.8)
project(SSESwizzle)

set(SOURCES 
    SSESwizzle.cpp)

set(HEADERS)

add_executable(SSESwizzle 
    ${SOURCES} 
    ${HEADERS})

SET(PLATFORM_LIBS)
if(WIN32)
	SET(PLATFORM_LIBS)
endif()

target_link_libraries(SSESwizzle
	${PLATFORM_LIBS})