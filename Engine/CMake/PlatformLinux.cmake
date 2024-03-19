#set platform related cmake definition and config

message(">>>>>>>>>>>> LINUX PLATFORM ENGINE ABI: " ${ENGINE_ABI})
message(">>>>>>>>>>>> LINUX PLATFORM ENGINE SOURCE: " ${ENGINE_SOURCE_DIR})
message(">>>>>>>>>>>> LINUX PLATFORM ENGINE THIRDPARTY: " ${ENGINE_THIRDPARTY_DIR})

#指定动态链接库的查找路径
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "./libs")
#-zmuldefs
set(PLATFORM_CXX_FLAGS "${PLATFORM_CXX_FLAGS} -Wno-inconsistent-missing-override \
-fvisibility=hidden \
-Wno-switch \
-Wno-undefined-var-template \
-std=c++17 \
-pthread \
-frtti \
-fexceptions \
-Wreturn-type \
-Wdelete-incomplete \
-Wno-error=format-security \
-Wno-undefined-var-templat \
-fms-extensions \
-fomit-frame-pointer"
)
add_definitions(-DPLATFORM_LINUX=1)
if(${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
    message("[D] BUILD TYPE IS " ${CMAKE_BUILD_TYPE})
else()
    message("[R] BUILD TYPE IS " ${CMAKE_BUILD_TYPE})

    set(PLATFORM_CXX_FLAGS "${PLATFORM_CXX_FLAGS} -O3")
        # -s选项为strip，不strip生成的库文件会很大
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
endif()

if(NOT NS_DOUBLE_SUPPORT)
    set(PLATFORM_CXX_FLAGS "${PLATFORM_CXX_FLAGS} -ffast-math -fno-finite-math-only")
endif()
#-fomit-frame-pointer \
#-ffast-math \
#-fno-finite-math-only \
#-flto \
#-fdata-sections \
#-ffunction-sections"

set(PLATFORM_SOURCE 
        #Platform/Linux
        #${ENGINE_SOURCE_DIR}/Platform/Linux/LinuxFile.cpp
        ${ENGINE_SOURCE_DIR}/Application/Linux/LinuxWindow.cpp
		${ENGINE_SOURCE_DIR}/Application/Linux/LinuxApplication.cpp
		${ENGINE_SOURCE_DIR}/Vulkan/Linux/VulkanLinuxPlatform.cpp
		${ENGINE_SOURCE_DIR}/GenericPlatform/Linux/LinuxPlatformTime.cpp
		${ENGINE_SOURCE_DIR}/GenericPlatform/Linux/InputManagerLinux.cpp
        )

set(PLATFORM_INCLUDES 
        ${ENGINE_THIRDPARTY_DIR}/vulkan/linux/include
        #${ENGINE_THIRDPARTY_DIR}/OpenGL/GLAD/include
)

if(ENGINE_DEBUG)
#Physx4.1使用DEBUG库需要声明-DNDEBUG
#定义后每个GL CALL会进行ERROR检查
    list(INSERT PLATFORM_DEFINITIONS 0 "DEBUG_OPENGL" "NDEBUG")
else()
endif(ENGINE_DEBUG)

set(PLATFORM_LINK_LIBS 
   ${XCB_LIBRARIES}
   )

if( "${ENGINE_ABI}" STREQUAL "linux-x64" )
    list(APPEND PLATFORM_LINK_LIBS idn2)
endif()

find_package(XCB REQUIRED)
	include_directories(
		${XCB_INCLUDE_DIRS}
	)

list(APPEND ENGINE_LINK_LIBS )

 message(">>>>>>>>> Platform link libs: " ${PLATFORM_LINK_LIBS})
message(">>>>>>>>> Engine link libs: " ${ENGINE_LINK_LIBS})
