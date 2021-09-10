cmake_minimum_required(VERSION 3.2)
project(asmlib VERSION 2.52.0.0)

set(_LIB "")

# Detect Architecture
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
if(WIN32)
	set(_LIB "${PROJECT_SOURCE_DIR}/libacof${BITS}.lib")
elseif(UNIX)
	set(_LIB "${PROJECT_SOURCE_DIR}/libaelf${BITS}.lib")
elseif(MACOSX)
	set(_LIB "${PROJECT_SOURCE_DIR}/libamac${BITS}.lib")
endif()
#message("asmlib: Using static library '${_LIB}'")

add_library(asmlib STATIC IMPORTED GLOBAL)
set_target_properties(${PROJECT_NAME}
	PROPERTIES
		PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/asmlib.h"
		IMPORTED_LOCATION "${_LIB}"
)

target_include_directories(${PROJECT_NAME}
	INTERFACE
		"${PROJECT_SOURCE_DIR}"
)
target_link_libraries(${PROJECT_NAME}
	INTERFACE
		"${_LIB}"
)
