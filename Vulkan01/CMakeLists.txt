cmake_minimum_required(VERSION 2.8)
project(Vulkan01)

set(SOURCES 
    main.cpp)

set(HEADERS)

add_executable(Vulkan01 
    ${SOURCES} 
    ${HEADERS})

SET(PLATFORM_LIBS)
if(WIN32)
	SET(PLATFORM_LIBS)
endif()

target_link_libraries(Vulkan01
	${PLATFORM_LIBS})