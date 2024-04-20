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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/")
#link_directories(${CMAKE_SOURCE_DIR}/Libs)

set(ROOT_PATH ${PROJECT_SOURCE_DIR})
set(ENGINE_DIR ${ROOT_PATH})
set(ENGINE_INCLUDE_DIR ${ENGINE_DIR}/Include)
set(ENGINE_SOURCE_DIR ${ENGINE_DIR}/Source)
set(ENGINE_THIRDPARTY_DIR ${ENGINE_DIR}/Libs)
set(ENGINE_TARGET_OS ${TARGET_OS})
set(ENGINE_TARGET_ARCH ${TARGET_ARCH})

add_definitions(-DROOT_PATH=\"${ENGINE_DIR}/\")
add_definitions(-DNOMINMAX=1)
add_definitions(-DMONKEY_DEBUG=1)
add_definitions(-DDEMO_RES_PATH=\"${ROOT_PATH}/examples/\")

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

if ("${ENGINE_TARGET_OS}" STREQUAL "linux")
include(CMake/PlatformLinux.cmake)
elseif("${ENGINE_TARGET_OS}" STREQUAL "windows")
include(CMake/PlatformWindows.cmake)
elseif("${ENGINE_TARGET_OS}" STREQUAL "android")
include(CMake/PlatformAndroid.cmake)
endif()

include_directories(${ROOT_PATH}/Include)
include_directories(${ROOT_PATH}/Source)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PLATFORM_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PLATFORM_LINK_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${PLATFORM_LINK_FLAGS}")

# AUTO_LOCATE_VULKAN - accepted value ON or OFF
# ON  - Use CMake to auto locate the Vulkan SDK.
# OFF - Vulkan SDK path can be specified manually. This is helpful to test the build on various Vulkan version.
#option(AUTO_LOCATE_VULKAN "AUTO_LOCATE_VULKAN" OFF)
#
#if(AUTO_LOCATE_VULKAN)
#	message(STATUS "Attempting auto locate Vulkan using CMake......")
#	# Find Vulkan Path using CMake's Vulkan Module
#	# This will return Boolean 'Vulkan_FOUND' indicating the status of find as success(ON) or fail(OFF).
#	# Include directory path - 'Vulkan_INCLUDE_DIRS' and 'Vulkan_LIBRARY' with required libraries.
#	find_package(Vulkan)
#	
#	# Try extracting VulkanSDK path from ${Vulkan_INCLUDE_DIRS}
#	if (NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "")
#		set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
#		STRING(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})
#	endif()
#	
#	if(NOT Vulkan_FOUND)
#		# CMake may fail to locate the libraries but could be able to 
#		# provide some path in Vulkan SDK include directory variable
#		# 'Vulkan_INCLUDE_DIRS', try to extract path from this.
#		message(STATUS "Failed to locate Vulkan SDK, retrying again...")
#		if(EXISTS "${VULKAN_PATH}")
#			message(STATUS "Successfully located the Vulkan SDK: ${VULKAN_PATH}")
#		else()
#			message("Error: Unable to locate Vulkan SDK. Please turn off auto locate option by specifying 'AUTO_LOCATE_VULKAN' as 'OFF'")
#			message("and specify manually path using 'VULKAN_SDK' and 'VULKAN_VERSION' variables in the CMakeLists.txt.")
#			return()
#		endif()
#	endif()
#endif()

# BUILD_SPV_ON_COMPILE_TIME - accepted value ON or OFF, default value OFF.
# ON  - Reads the GLSL shader file and auto convert in SPIR-V form (.spv). 
# 			This requires additional libraries support from 
#			VulkanSDK like SPIRV glslang OGLCompiler OSDependent HLSL
# OFF - Only reads .spv files, which need to be compiled offline 
#			using glslangValidator.exe.
# For example: glslangValidator.exe <GLSL file name> -V -o <output filename in SPIR-V(.spv) form>
#option(BUILD_SPV_ON_COMPILE_TIME "BUILD_SPV_ON_COMPILE_TIME" OFF)

# Add any required preprocessor definitions here
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)

# We do not use ${Vulkan_LIBRARY}, instead we specify as per our need.
# Add 'vulkan-1' library for building Vulkan applications.
#set(VULKAN_LIB_LINK_LIST "vulkan-1")
#if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
#	# Include Vulkan header files from Vulkan SDK
#	include_directories(AFTER ${VULKAN_PATH}/Include)
#	# Link directory for vulkan-1
#	link_directories(${VULKAN_PATH}/Bin)
#	#是否支持在线编译shader为spri-v
#	if(BUILD_SPV_ON_COMPILE_TIME)
#		# Preprocessor  flag allows the solution to use glslang library functions
#		add_definitions(-DAUTO_COMPILE_GLSL_TO_SPV)
#		#GLSL - use Vulkan SDK's glslang library for compling GLSL to SPV 
#		# This does not require offline coversion of GLSL shader to SPIR-V(.spv) form 
#		set(GLSLANGDIR "${VULKAN_PATH}/glslang")
#		get_filename_component(GLSLANG_PREFIX "${GLSLANGDIR}" ABSOLUTE)
#		if(NOT EXISTS ${GLSLANG_PREFIX})
#			message(FATAL_ERROR "Necessary glslang components do not exist: " ${GLSLANG_PREFIX})
#		endif()
#		include_directories( ${GLSLANG_PREFIX} )
#		
#		# If compiling GLSL to SPV using we need the following libraries
#		set(GLSLANG_LIBS SPIRV glslang OGLCompiler OSDependent HLSL)
#
#		# Generate the list of files to link, per flavor.
#		foreach(x ${GLSLANG_LIBS})
#			list(APPEND VULKAN_LIB_LINK_LIST debug ${x}d optimized ${x})
#		endforeach()
#		# Note: While configuring CMake for glslang we created the binaries in a "build" folder inside ${VULKAN_PATH}/glslang.
#		# Therefore, you must edit the below lines for your custorm path like <Your binary path>/OGLCompilersDLL , <Your binary path>/OSDependent/Windows
#		link_directories(${VULKAN_PATH}/glslang/build/OGLCompilersDLL )
#		link_directories(${VULKAN_PATH}/glslang/build/glslang/OSDependent/Windows)
#		link_directories(${VULKAN_PATH}/glslang/build/glslang)
#		link_directories(${VULKAN_PATH}/glslang/build/SPIRV)
#		link_directories(${VULKAN_PATH}/glslang/build/hlsl)
#	endif()
#endif()

set(WKEngine_Sources
		${ENGINE_SOURCE_DIR}/Graphics/ModuleBase.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKBuffer.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKCommand.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKVertexBuffer.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKIndexBuffer.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKModel.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKPipeline.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKTexture.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKShader.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKMaterial.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKDefaultRes.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKRenderTarget.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKCamera.cpp
		${ENGINE_SOURCE_DIR}/Graphics/DVKCompute.cpp
		${ENGINE_SOURCE_DIR}/Graphics/ImageGUIContext.cpp

		${ENGINE_SOURCE_DIR}/File/FileManager.cpp
		${ENGINE_SOURCE_DIR}/Engine.cpp
		${ENGINE_SOURCE_DIR}/Launch/Launch.cpp
		${ENGINE_SOURCE_DIR}/Application/GenericWindow.cpp
		${ENGINE_SOURCE_DIR}/Application/GenericApplication.cpp
		${ENGINE_SOURCE_DIR}/Application/Application.cpp
		${ENGINE_SOURCE_DIR}/Math/Math.cpp
		${ENGINE_SOURCE_DIR}/Math/GenericPlatformMath.cpp
		${ENGINE_SOURCE_DIR}/Math/Color.cpp
		${ENGINE_SOURCE_DIR}/Utils/SecureHash.cpp
		${ENGINE_SOURCE_DIR}/Utils/Crc.cpp
		${ENGINE_SOURCE_DIR}/Core/PixelFormat.cpp
		${ENGINE_SOURCE_DIR}/Vulkan/VulkanRHI.cpp
		${ENGINE_SOURCE_DIR}/Vulkan/VulkanLayers.cpp
		${ENGINE_SOURCE_DIR}/Vulkan/VulkanQueue.cpp
		${ENGINE_SOURCE_DIR}/Vulkan/VulkanDevice.cpp
		${ENGINE_SOURCE_DIR}/Vulkan/VulkanDebug.cpp
		${ENGINE_SOURCE_DIR}/Vulkan/VulkanSwapChain.cpp
		${ENGINE_SOURCE_DIR}/Vulkan/VulkanMemory.cpp
		${ENGINE_SOURCE_DIR}/Vulkan/VulkanFence.cpp
		${ENGINE_SOURCE_DIR}/Loader/ImageLoader.cpp
		${ENGINE_SOURCE_DIR}/GenericPlatform/InputManager.cpp
		
        ${PLATFORM_SOURCE}
		)

set(58_CPURayTracing_Source
   ${ROOT_PATH}/examples/58_CPURayTracing/CPURayTracingDemo.cpp
   ${ROOT_PATH}/examples/58_CPURayTracing/Material.cpp
   ${ROOT_PATH}/examples/58_CPURayTracing/RayTracing.cpp
   ${ROOT_PATH}/examples/58_CPURayTracing/RunnableThread.cpp
   ${ROOT_PATH}/examples/58_CPURayTracing/TaskThread.cpp
   ${ROOT_PATH}/examples/58_CPURayTracing/TaskThreadPool.cpp
   ${ROOT_PATH}/examples/58_CPURayTracing/ThreadEvent.cpp
   ${ROOT_PATH}/examples/58_CPURayTracing/ThreadManager.cpp
)
set(Examples_Source
	#${ROOT_PATH}/examples/11_TextureArray/TextureArray.cpp
	#${ROOT_PATH}/examples/12_Texture3D/Texture3D.cpp
	#${ROOT_PATH}/examples/13_OptimizeShaderAndLayout/OptimizeShaderAndLayout.cpp
	#${ROOT_PATH}/examples/14_InputAttachments/InputAttachments.cpp
	#${ROOT_PATH}/examples/15_DeferredShading/DeferredShading.cpp
	#${ROOT_PATH}/examples/16_OptimizeDeferredShading/OptimizeDeferredShading.cpp
	#${ROOT_PATH}/examples/17_Material/MaterialDemo.cpp
	#${ROOT_PATH}/examples/18_Stencil/StencilDemo.cpp
	#${ROOT_PATH}/examples/19_RenderTarget/RenderTargetDemo.cpp
	#${ROOT_PATH}/examples/20_OptimizeRenderTarget/OptimizeRenderTarget.cpp
	#${ROOT_PATH}/examples/21_EdgeDetect/EdgeDetectDemo.cpp
	#${ROOT_PATH}/examples/22_Bloom/BloomDemo.cpp
	#${ROOT_PATH}/examples/23_SkeletonMatrix4x4/SkeletonMatrix4x4Demo.cpp
	#${ROOT_PATH}/examples/24_SkeletonPackIndexWeight/SkeletonPackIndexWeightDemo.cpp
	#${ROOT_PATH}/examples/25_SkeletonQuat/SkeletonQuatDemo.cpp
	#${ROOT_PATH}/examples/26_SkinInTexture/SkinInTextureDemo.cpp
	#${ROOT_PATH}/examples/27_SkinInstance/SkinInstanceDemo.cpp
	#${ROOT_PATH}/examples/29_FXAA/MSAADemo.cpp
	#${ROOT_PATH}/examples/29_FXAA/FXAADemo.cpp
	#${ROOT_PATH}/examples/30_InstanceDraw/InstanceDrawDemo.cpp
	#${ROOT_PATH}/examples/31_SimpleShadow/SimpleShadowDemo.cpp
	#${ROOT_PATH}/examples/32_PCFShadow/PCFShadowDemo.cpp
	#${ROOT_PATH}/examples/34_CascadedShadow/CascadedShadowDemo.cpp
	#${ROOT_PATH}/examples/35_IndirectDraw/IndirectDrawDemo.cpp
	#${ROOT_PATH}/examples/36_OcclusionQueries/OcclusionQueryDemo.cpp
	#${ROOT_PATH}/examples/37_QueryStatistics/QueryStatisticsDemo.cpp
	#${ROOT_PATH}/examples/38_ComputeShader/ComputeShaderDemo.cpp
	#${ROOT_PATH}/examples/39_OptimizeComputeShader/OptimizeComputeShaderDemo.cpp
	#${ROOT_PATH}/examples/41_ComputeRaytracing/ComputeRaytracingDemo.cpp
	#${ROOT_PATH}/examples/42_ComputeFrustum/ComputeFrustumDemo.cpp
	#${ROOT_PATH}/examples/43_GeometryHouse/GeometryHouseDemo.cpp
	#${ROOT_PATH}/examples/44_DebugNormal/DebugNormalDemo.cpp
	#${ROOT_PATH}/examples/45_GeometryOmniShadow/GeometryOmniShadowDemo.cpp
	#${ROOT_PATH}/examples/46_SimpleTessellation/SimpleTessellationDemo.cpp
	#${ROOT_PATH}/examples/47_PNTessellation/PNTessellationDemo.cpp
	#${ROOT_PATH}/examples/48_Pick/PickDemo.cpp
	#${ROOT_PATH}/examples/49_HDRPipeline/HDRPipelineDemo.cpp
	#${ROOT_PATH}/examples/50_SSAO/SSAODemo.cpp
	#${ROOT_PATH}/examples/51_ThreadedRendering/ThreadedRenderingDemo.cpp
	#${ROOT_PATH}/examples/52_PBR_DirectLighting/PBRDirectLightingDemo.cpp
	#${ROOT_PATH}/examples/53_PBR_IBL/PBRIBLDemo.cpp
	#${ROOT_PATH}/examples/54_GodRay/GodRayDemo.cpp
	#${ROOT_PATH}/examples/55_Imposter/ImposterDemo.cpp
	#${ROOT_PATH}/examples/56_MotionBlur/MotionBlurDemo.cpp
	#${ROOT_PATH}/examples/57_DepthPeeling/DepthPeelingDemo.cpp
	#${58_CPURayTracing_Source}
	#${ROOT_PATH}/examples/59_RTXRayTracingBasic/RTXRayTracingStartBasic.cpp
	#${ROOT_PATH}/examples/60_RTXRayTracingMesh/RTXRayTracingMeshDemo.cpp
	#${ROOT_PATH}/examples/61_RTXRayTracingSimple/RTXRayTracingSimpleDemo.cpp
	#${ROOT_PATH}/examples/62_RTXRayTracingReflection/RTXRayTracingReflectionDemo.cpp
	#${ROOT_PATH}/examples/63_RTXRayTracingHitGroup/RTXRayTracingHitGroupDemo.cpp
	#${ROOT_PATH}/examples/64_RTXRayTracingMonteCarlo/RTXRayTracingMonteCarloDemo.cpp
	${ROOT_PATH}/examples/65_RTXPathTracing/RTXPathTracingDemo.cpp

)

if("${ENGINE_TARGET_OS}" STREQUAL "windows")
        #指定为桌面程序
        set(EXE_TARGET WIN32)
endif()

add_executable(WKEngine ${EXE_TARGET} ${WKEngine_Sources} ${PLATFORM_SOURCE} ${Examples_Source})

message("ENGINE_THIRDPARTY_DIR: " ${ENGINE_THIRDPARTY_DIR})
message("ENGINE_INCLUDE_DIR: " ${ENGINE_INCLUDE_DIR})

target_include_directories(WKEngine
        PUBLIC
		${ENGINE_DIR}
        ${ENGINE_INCLUDE_DIR}/gli/
        ${ENGINE_INCLUDE_DIR}/glm/
		${PLATFORM_INCLUDES}
)
# Link the debug and release libraries to the project
target_link_libraries( WKEngine PUBLIC ${ENGINE_LINK_LIBS} ${VULKAN_LIB_LINK_LIST} ${ALL_LIBS})
