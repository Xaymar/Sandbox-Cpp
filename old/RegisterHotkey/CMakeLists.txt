cmake_minimum_required(VERSION 2.8)
project(RegisterHotkey)

set(SOURCES 
    main.cpp)

set(HEADERS)

add_executable(RegisterHotkey 
    ${SOURCES} 
    ${HEADERS})

SET(PLATFORM_LIBS)
if(WIN32)
	SET(PLATFORM_LIBS)
endif()

target_link_libraries(RegisterHotkey
	${PLATFORM_LIBS})