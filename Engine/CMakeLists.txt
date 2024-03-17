message("TARGET_OS: " ${TARGET_OS})
if("${TARGET_OS}" STREQUAL "windows")
cmake_minimum_required(VERSION 3.16)
else()
cmake_minimum_required(VERSION 3.8)
endif()

project(KKEngine LANGUAGES CXX)
message("PROJECT_NAME: " ${PROJECT_NAME})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(NS_DEBUG_PROJECT "Debug Project" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/modules/")
#link_directories(${CMAKE_SOURCE_DIR}/Libs)

set(ROOT_PATH ${PROJECT_SOURCE_DIR})
set(ENGINE_DIR ${ROOT_PATH})
set(ENGINE_INCLUDE_DIR ${ENGINE_DIR}/Include)
set(ENGINE_SOURCE_DIR ${ENGINE_DIR}/Source)
set(ENGINE_THIRDPARTY_DIR ${ENGINE_DIR}/Libs)
set(ENGINE_TARGET_OS ${TARGET_OS})
set(ENGINE_TARGET_ARCH ${TARGET_ARCH})

add_definitions(-DROOT_PATH=\"${ENGINE_DIR}/\")

if("${ENGINE_TARGET_OS}" STREQUAL "android")
    set(ENGINE_TARGET_ARCH ${ANDROID_ABI})
endif()

set(ENGINE_ABI ${ENGINE_TARGET_OS}-${ENGINE_TARGET_ARCH})

message(">>>>>>>>> CMAKE VERSION is " ${CMAKE_VERSION})
message(">>>>>>>>> COMPILER C PATH is " ${CMAKE_C_COMPILER})
message(">>>>>>>>> COMPILER C++ PATH is " ${CMAKE_CXX_COMPILER})
message(">>>>>>>>> ROOT_PATH is " ${ROOT_PATH})
message(">>>>>>>>> PROJECT_SOURCE_DIR is " ${PROJECT_SOURCE_DIR})
message(">>>>>>>>> PROJECT_BINARY_DIR is " ${PROJECT_BINARY_DIR})
message(">>>>>>>>> BUILD TYPE is " ${CMAKE_BUILD_TYPE})
message(">>>>>>>>> COMPILE HOST ARCHTECTURE: " ${CMAKE_HOST_SYSTEM_PROCESSOR})
message(">>>>>>>>> ENGINE_TARGET_OS: " ${ENGINE_TARGET_OS})
message(">>>>>>>>> ENGINE_TARGET_ARCH: " ${ENGINE_TARGET_ARCH})
message(">>>>>>>>> ENGINE ABI: " ${ENGINE_ABI})
message(">>>>>>>>> NS_DEBUG_PROJECT: " ${NS_DEBUG_PROJECT})


if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(ENGINE_DEBUG ON CACHE INTERNAL "debug")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    set(ENGINE_DEBUG OFF CACHE INTERNAL "release")
else()
    set(ENGINE_DEBUG OFF CACHE INTERNAL "release")
endif()

include_directories(${ROOT_PATH}/Include)
include_directories(${ROOT_PATH}/Source)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PLATFORM_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PLATFORM_LINK_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${PLATFORM_LINK_FLAGS}")

message("CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
message("CMAKE_CXX_FLAGS: " ${ENGINE_SOURCE_DIR})

# AUTO_LOCATE_VULKAN - accepted value ON or OFF
# ON  - Use CMake to auto locate the Vulkan SDK.
# OFF - Vulkan SDK path can be specified manually. This is helpful to test the build on various Vulkan version.
option(AUTO_LOCATE_VULKAN "AUTO_LOCATE_VULKAN" ON)

if(AUTO_LOCATE_VULKAN)
	message(STATUS "Attempting auto locate Vulkan using CMake......")
	# Find Vulkan Path using CMake's Vulkan Module
	# This will return Boolean 'Vulkan_FOUND' indicating the status of find as success(ON) or fail(OFF).
	# Include directory path - 'Vulkan_INCLUDE_DIRS' and 'Vulkan_LIBRARY' with required libraries.
	find_package(Vulkan)
	
	# Try extracting VulkanSDK path from ${Vulkan_INCLUDE_DIRS}
	if (NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "")
		set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
		STRING(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})
	endif()
	
	if(NOT Vulkan_FOUND)
		# CMake may fail to locate the libraries but could be able to 
		# provide some path in Vulkan SDK include directory variable
		# 'Vulkan_INCLUDE_DIRS', try to extract path from this.
		message(STATUS "Failed to locate Vulkan SDK, retrying again...")
		if(EXISTS "${VULKAN_PATH}")
			message(STATUS "Successfully located the Vulkan SDK: ${VULKAN_PATH}")
		else()
			message("Error: Unable to locate Vulkan SDK. Please turn off auto locate option by specifying 'AUTO_LOCATE_VULKAN' as 'OFF'")
			message("and specify manually path using 'VULKAN_SDK' and 'VULKAN_VERSION' variables in the CMakeLists.txt.")
			return()
		endif()
	endif()
endif()

# BUILD_SPV_ON_COMPILE_TIME - accepted value ON or OFF, default value OFF.
# ON  - Reads the GLSL shader file and auto convert in SPIR-V form (.spv). 
# 			This requires additional libraries support from 
#			VulkanSDK like SPIRV glslang OGLCompiler OSDependent HLSL
# OFF - Only reads .spv files, which need to be compiled offline 
#			using glslangValidator.exe.
# For example: glslangValidator.exe <GLSL file name> -V -o <output filename in SPIR-V(.spv) form>
option(BUILD_SPV_ON_COMPILE_TIME "BUILD_SPV_ON_COMPILE_TIME" OFF)

# Add any required preprocessor definitions here
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)

# We do not use ${Vulkan_LIBRARY}, instead we specify as per our need.
# Add 'vulkan-1' library for building Vulkan applications.
set(VULKAN_LIB_LINK_LIST "vulkan-1")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	# Include Vulkan header files from Vulkan SDK
	include_directories(AFTER ${VULKAN_PATH}/Include)
	# Link directory for vulkan-1
	link_directories(${VULKAN_PATH}/Bin)
	#是否支持在线编译shader为spri-v
	if(BUILD_SPV_ON_COMPILE_TIME)
		# Preprocessor  flag allows the solution to use glslang library functions
		add_definitions(-DAUTO_COMPILE_GLSL_TO_SPV)
		#GLSL - use Vulkan SDK's glslang library for compling GLSL to SPV 
		# This does not require offline coversion of GLSL shader to SPIR-V(.spv) form 
		set(GLSLANGDIR "${VULKAN_PATH}/glslang")
		get_filename_component(GLSLANG_PREFIX "${GLSLANGDIR}" ABSOLUTE)
		if(NOT EXISTS ${GLSLANG_PREFIX})
			message(FATAL_ERROR "Necessary glslang components do not exist: " ${GLSLANG_PREFIX})
		endif()
		include_directories( ${GLSLANG_PREFIX} )
		
		# If compiling GLSL to SPV using we need the following libraries
		set(GLSLANG_LIBS SPIRV glslang OGLCompiler OSDependent HLSL)

		# Generate the list of files to link, per flavor.
		foreach(x ${GLSLANG_LIBS})
			list(APPEND VULKAN_LIB_LINK_LIST debug ${x}d optimized ${x})
		endforeach()
		# Note: While configuring CMake for glslang we created the binaries in a "build" folder inside ${VULKAN_PATH}/glslang.
		# Therefore, you must edit the below lines for your custorm path like <Your binary path>/OGLCompilersDLL , <Your binary path>/OSDependent/Windows
		link_directories(${VULKAN_PATH}/glslang/build/OGLCompilersDLL )
		link_directories(${VULKAN_PATH}/glslang/build/glslang/OSDependent/Windows)
		link_directories(${VULKAN_PATH}/glslang/build/glslang)
		link_directories(${VULKAN_PATH}/glslang/build/SPIRV)
		link_directories(${VULKAN_PATH}/glslang/build/hlsl)
	endif()
endif()

set(WKEngine_Sources
        ${ENGINE_SOURCE_DIR}/Core/VulkanSwapChain.cpp
        ${ENGINE_SOURCE_DIR}/Core/VulkanShader.cpp
		 ${ENGINE_SOURCE_DIR}/Core/VulkanInstance.cpp
        ${ENGINE_SOURCE_DIR}/Core/VulkanRenderer.cpp
        ${ENGINE_SOURCE_DIR}/Core/VulkanPipeline.cpp
        ${ENGINE_SOURCE_DIR}/Core/VulkanLayerAndExtension.cpp
        ${ENGINE_SOURCE_DIR}/Core/VulkanDrawable.cpp
        ${ENGINE_SOURCE_DIR}/Core/VulkanDevice.cpp
        ${ENGINE_SOURCE_DIR}/Core/VulkanDescriptor.cpp
        ${ENGINE_SOURCE_DIR}/Core/CommandBufferMgr.cpp
        ${ENGINE_SOURCE_DIR}/Core/VulkanApplication.cpp
        ${ENGINE_SOURCE_DIR}/Platforms/WinMain.cpp
		    #platform related
        ${PLATFORM_SOURCE}
		)

if("${ENGINE_TARGET_OS}" STREQUAL "windows")
        #指定为桌面程序
        set(EXE_TARGET WIN32)
endif()

add_executable(WKEngine ${EXE_TARGET} ${WKEngine_Sources} ${PLATFORM_TEST_RUN})

message("ENGINE_THIRDPARTY_DIR: " ${ENGINE_THIRDPARTY_DIR})

target_include_directories(WKEngine
        PUBLIC
		${ENGINE_DIR}
        ${ENGINE_THIRDPARTY_DIR}/gli/
        ${ENGINE_THIRDPARTY_DIR}/glm/
)
# Link the debug and release libraries to the project
target_link_libraries( WKEngine PUBLIC ${ENGINE_LINK_LIBS} ${VULKAN_LIB_LINK_LIST} )
