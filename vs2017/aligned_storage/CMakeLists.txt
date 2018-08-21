cmake_minimum_required(VERSION 2.8)
project(vs2017_aligned-storage)

set(SOURCES 
    main.cpp)

set(HEADERS)

add_executable(${PROJECT_NAME}
    ${SOURCES} 
    ${HEADERS})

SET(PLATFORM_LIBS)
if(WIN32)
	SET(PLATFORM_LIBS)
endif()

target_link_libraries(${PROJECT_NAME}
	${PLATFORM_LIBS})