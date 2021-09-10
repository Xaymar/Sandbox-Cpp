project(
	nvenc-profile-extractor
	VERSION 0.0.0.0
)

set(SOURCES 
    main.cpp)

set(HEADERS)

set(VIDEO_CODEC_SDK "" CACHE PATH "Path to NVIDIA Video Codec SDK")

add_executable(${PROJECT_NAME}
    ${SOURCES} 
    ${HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${VIDEO_CODEC_SDK}/Interface"
)

target_link_libraries(
	${PROJECT_NAME}
	utility-profiler
	"${VIDEO_CODEC_SDK}/Lib/x64/nvcuvid.lib"
	"${VIDEO_CODEC_SDK}/Lib/x64/nvencodeapi.lib"
)
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF	
)
