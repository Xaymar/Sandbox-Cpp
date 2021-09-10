project(
	benchmark_core2corelatency
	VERSION 0.0.0.0
)

add_executable(${PROJECT_NAME}
    main.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
target_link_libraries(
	${PROJECT_NAME}
	profiler
)
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF	
)
