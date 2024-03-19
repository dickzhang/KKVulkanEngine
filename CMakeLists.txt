# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。

cmake_minimum_required (VERSION 3.8)
project ("KKVulkanEngine")

add_definitions(-DNOMINMAX=1)
add_definitions(-DMONKEY_DEBUG=1)

if (WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DPLATFORM_WINDOWS=1)
	include_directories(
		external/vulkan/windows/include/
	)
elseif (APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPLATFORM_MAC=1")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework QuartzCore")
	set(CMAKE_XCODE_GENERATE_SCHEME ON)
	set(XCODE_SCHEME_ENVIRONMENT ON)
	include_directories(
		external/vulkan/macos/include/
	)
elseif (UNIX)
	add_definitions(-DPLATFORM_LINUX=1)
	include_directories(
		external/vulkan/linux/include/
	)
	find_package(XCB REQUIRED)
	include_directories(
		${XCB_INCLUDE_DIRS}
	)
endif ()

find_package(Vulkan)

set(ALL_LIBS
	${Vulkan_LIBRARY}
	imgui
	assimp
	spirv-cross-glsl
	spirv-cross-hlsl
	spirv-cross-cpp
	spirv-cross-reflect
	spirv-cross-msl
	spirv-cross-util
	spirv-cross-core
)

if (UNIX AND NOT APPLE)
	set(ALL_LIBS
		${ALL_LIBS}
		${XCB_LIBRARIES}
	)
endif ()

include_directories(
	Engine/
	external/imgui/
	external/SPIRV-Cross/
	external/assimp/include/
)

# 包含子项目。
add_subdirectory ("Engine")
add_subdirectory("external/imgui")
add_subdirectory("external/SPIRV-Cross")
add_subdirectory("external/assimp")

if( ${NS_DEBUG_PROJECT} AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
#调试项目，需要在CMakeSettings.json中改为-DNS_DEBUG_PROJECT=ON，注意RELEASE下自动关闭项目调试，防止项目库被错误发布
#add_subdirectory ("DebugProjects/NewProject")
endif()