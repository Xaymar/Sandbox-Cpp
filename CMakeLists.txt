cmake_minimum_required(VERSION 3.12.0)

cmake_policy(VERSION 3.12)
cmake_policy(SET CMP0048 NEW)

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

# Benchmarks
add_subdirectory(benchmarks)
