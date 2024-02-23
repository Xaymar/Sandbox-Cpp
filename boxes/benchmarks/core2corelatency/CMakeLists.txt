project(
	benchmark_core2corelatency
	VERSION 0.0.0.0
	LANGUAGES C
)

set(PROJECT_ASSEMBLY
	measure.asm
)

# Define Binary
add_executable(${PROJECT_NAME}
    main.cpp
	${PROJECT_ASSEMBLY}
)

# Build assembly shit
if(WIN32)
	math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
	if(BITS EQUAL 64)
		set(NASM_FORMAT "win64")
	else()
		set(NASM_FORMAT "win32")
	endif()
	set(NASM_SUFFIX "obj")
elseif(UNIX)
	set(NASM_FORMAT "aout")
	set(NASM_SUFFIX "a")
endif()

set(OBJECTS)
foreach(ASM IN LISTS PROJECT_ASSEMBLY)
	get_filename_component(ASM_DIR "${ASM}" DIRECTORY)
	get_filename_component(ASM_FILE "${ASM}" NAME_WE)
	get_filename_component(ASM_EXT "${ASM}" EXT)
	set(ASM_OUT "${PROJECT_BINARY_DIR}/asm.dir/${ASM_DIR}/${ASM_FILE}.${NASM_SUFFIX}")

	list(APPEND OBJECTS ${ASM_OUT})

	file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/asm.dir/${ASM_DIR}")
	file(REAL_PATH "${ASM}" ASM)
	file(TO_NATIVE_PATH "${ASM}" ASM_)
	file(TO_NATIVE_PATH "${ASM_OUT}" ASM_OUT_)
	file(TO_NATIVE_PATH "${ASSEMBLER}" ASSEMBLER_)

	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
		COMMAND "${ASSEMBLER_}" ARGS -f ${NASM_FORMAT} -o "${ASM_OUT_}" "${ASM_}"
		BYPRODUCTS ${ASM_OUT}
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
		COMMENT "Compiling '${ASM}'..."
	)
	message(STATUS "Added Assembly step for ${ASM}...")
endforeach()

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
target_link_libraries(
	${PROJECT_NAME}
	${OBJECTS}
	xmr_utility_profiler
)
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
