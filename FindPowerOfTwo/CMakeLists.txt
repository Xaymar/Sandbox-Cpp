cmake_minimum_required(VERSION 2.8)
project(FindPowerOfTwo)

set(SOURCES 
    main.cpp)

set(HEADERS)

add_executable(FindPowerOfTwo 
    ${SOURCES} 
    ${HEADERS})

SET(PLATFORM_LIBS)
if(WIN32)
	SET(PLATFORM_LIBS winmm)
endif()

target_link_libraries(FindPowerOfTwo
	${PLATFORM_LIBS})