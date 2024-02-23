cmake_minimum_required(VERSION 3.2...4.0)
project(advmemcpy VERSION 1.0.0.0)

add_subdirectory(asmlib)

find_package (Threads)

set(HEADERS
    "os.hpp"
    "memcpy_adv.h"
)
set(SOURCES
    "main.cpp"
    "os.cpp"
    "memcpy_thread.cpp"
	"measurer.hpp"
	"measurer.cpp"
	"apex_memmove.h"
	"apex_memmove.c"
	"apex_memmove.cpp"
)

add_executable(advmemcpy
    ${SOURCES}
    ${HEADERS}
)

SET(PLATFORM_LIBS)
if(WIN32)
	SET(PLATFORM_LIBS
		winmm)
endif()

target_link_libraries(advmemcpy
    ${CMAKE_THREAD_LIBS_INIT}
	${PLATFORM_LIBS}
	asmlib
)
