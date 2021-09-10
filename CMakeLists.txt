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

# xmr::utility::profiler
add_subdirectory("thirdparty/xmr-utility-profiler")

# Sub-Projects
#add_subdirectory(advmemcpy)
#add_subdirectory(InvSqrt_Single)
#add_subdirectory(InvSqrt_Double)
#add_subdirectory(SSESwizzle)
#add_subdirectory(FindPowerOfTwo)
#add_subdirectory(RegisterHotkey)
#add_subdirectory(vs2017)
#add_subdirectory(GaussianMatrix)
#add_subdirectory(is_power_of_two)

# Vulkan Tests
#add_subdirectory(vulkan01)

# Tools
add_subdirectory(nvenc-profile-extractor)

# Benchmarks
add_subdirectory(benchmarks)
