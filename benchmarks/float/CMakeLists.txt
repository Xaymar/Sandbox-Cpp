project(
	benchmark-float
	VERSION 0.0.0.0
)

set(SOURCES 
    main.cpp)

set(HEADERS)

add_executable(${PROJECT_NAME}
    ${SOURCES} 
    ${HEADERS})

SET(PLATFORM_LIBS)
list(APPEND PLATFORM_LIBS
	profiler
)

target_link_libraries(
	${PROJECT_NAME}
	${PLATFORM_LIBS}
)
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF	
)

#foreach(FILE ${WINRING0_BIN})
#	add_custom_command(
#		TARGET ${PROJECT_NAME} POST_BUILD
#		COMMAND_EXPAND_LISTS 
#		COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different "${FILE}" "${PROJECT_BINARY_DIR}/$<CONFIG>/"
#	)
#endforeach()
