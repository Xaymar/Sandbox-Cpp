cmake_minimum_required(VERSION 2.8)
project(advmemcpy)

find_package (Threads)

set(SOURCES 
    main.cpp
    os.cpp
    memcpy_thread.cpp)

set(HEADERS 
    os.hpp
    memcpy_adv.h)

add_executable(advmemcpy 
    ${SOURCES} 
    ${HEADERS})

SET(PLATFORM_LIBS)
if(WIN32)
	SET(PLATFORM_LIBS
		winmm)
endif()
	
target_link_libraries(advmemcpy
    ${CMAKE_THREAD_LIBS_INIT}
	${PLATFORM_LIBS})