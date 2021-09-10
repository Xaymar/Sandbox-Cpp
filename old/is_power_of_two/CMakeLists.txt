cmake_minimum_required(VERSION 2.8)
project(is_power_of_two)

set(SOURCES 
    main.cpp)

set(HEADERS)

add_executable(${PROJECT_NAME}
    ${SOURCES} 
    ${HEADERS})

SET(PLATFORM_LIBS)
if(WIN32)
	SET(PLATFORM_LIBS winmm)
endif()

target_link_libraries(${PROJECT_NAME}
	${PLATFORM_LIBS})