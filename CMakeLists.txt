cmake_minimum_required(VERSION 3.12.0...3.999.999)
project(sandbox)

# Dependencies
## WinRing0
set(WINRING0_PATH "" CACHE PATH "Path to WinRing0 library and stuff.")
if(EXISTS ${WINRING0_PATH})
	set(WINRING0_INC "${WINRING0_PATH}/include")
	if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		set(WINRING0_LIB "${WINRING0_PATH}/bin/x86/WinRing0.lib")
		set(WINRING0_BIN "${WINRING0_PATH}/bin/x86/WinRing0x64.dll" "${WINRING0_PATH}/bin/x86/WinRing0x64.sys")
	else()
		set(WINRING0_LIB "${WINRING0_PATH}/bin/x86-64/WinRing0x64.lib")
		set(WINRING0_BIN "${WINRING0_PATH}/bin/x86-64/WinRing0x64.dll" "${WINRING0_PATH}/bin/x86-64/WinRing0x64.sys")
	endif()
endif()

# Third-Party
## xmr::utility::profiler
add_subdirectory("thirdparty/xmr-utility-profiler")

# Projects
function(has_parent_cmakelists DIR OUT)
	set("${OUT}" OFF PARENT_SCOPE)

	set(RDIR "${DIR}")
	while(NOT (RDIR STREQUAL ""))
		file(REAL_PATH "${RDIR}" RDIR BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
		file(RELATIVE_PATH RDIR "${CMAKE_CURRENT_SOURCE_DIR}" "${RDIR}/..")
		if(RDIR STREQUAL "")
			return()
		endif()

#		message(STATUS "${DIR}\n->${RDIR}")
		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${RDIR}/CMakeLists.txt")
			set("${OUT}" ON PARENT_SCOPE)
		endif()
	endwhile()
#	message("${OUT}=${${OUT}}")
endfunction()

file(GLOB_RECURSE DIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" LIST_DIRECTORIES true CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/boxes/*")
foreach(DIR ${DIRS})
#	message(STATUS "${DIR}")

#	message(STATUS "\\ Skip File?")
	if(NOT (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}"))
#		message(STATUS " \\- Yes")
		continue()
	endif()

#	message(STATUS " \\ Skip non-Project?")
	if(NOT (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/CMakeLists.txt"))
#		message(STATUS "  \\- Yes")
		continue()
	endif()

#	message(STATUS "  \\ Skip blacklisted?")
	file(REAL_PATH "${DIR}" ADIR BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
	if(ADIR MATCHES "${PROJECT_BINARY_DIR}")
#		message(STATUS "   \\- Yes")
		continue()
	endif()
	if(ADIR MATCHES "${PROJECT_SOURCE_DIR}/third-party")
#		message(STATUS "   \\- Yes")
		continue()
	endif()

#	message(STATUS "   \\ Skip child project?")
	has_parent_cmakelists(${DIR} STATUS)
	if(STATUS)
#		message(STATUS "    \\- Yes")
		continue()
	endif()

	message(STATUS "Adding: ${DIR}")
	add_subdirectory(${DIR})
endforeach()
