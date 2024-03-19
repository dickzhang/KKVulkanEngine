#set platform related cmake definition and config

message(">>>>>>>>>>>> ANDROID PLATFORM ENGINE ABI: " ${ENGINE_ABI})
message(">>>>>>>>>>>> ANDROID PLATFORM ENGINE SOURCE: " ${ENGINE_SOURCE_DIR})
message(">>>>>>>>>>>> ANDROID PLATFORM ENGINE THIRDPARTY: " ${ENGINE_THIRDPARTY_DIR})
message(">>>>>>>>>>>> ANDROID PLATFORM NDK: " ${ANDROID_NDK})

#NOTE: Android CMake CXX FLAGS and LINK FLAGS all setted in build.gradle. The following flags will be override by build.gradle cppFlags
#-fvisibility=hidden
#set(PLATFORM_CXX_FLAGS "${PLATFORM_CXX_FLAGS} -Wno-inconsistent-missing-override -Wreturn-type -Wdelete-incomplete -Wno-switch -Wno-undefined-var-template -Wno-error=format-security -fms-extensions -Wnull-character -std=c++17 -pthread  -frtti -fexceptions")
#set(PLATFORM_LINK_FLAGS "${PLATFORM_LINK_FLAGS} -rdynamic")
#set(VUFORIA_ENGINE_DIR ${ENGINE_THIRDPARTY_DIR}/Vuforia CACHE INTERNAL "vuforia lib")
set(NIBIRU_XR_SDK_DIR ${ENGINE_THIRDPARTY_DIR}/NibiruXR CACHE INTERNAL "nibiru xr lib")

#if(${ANDROID_ABI} STREQUAL "armeabi-v7a")
# make a list of neon files and add neon compiling flags to them
set(neon_SRCS ${ENGINE_SOURCE_DIR}/Math/Matrix4.cpp)
set_property(SOURCE ${neon_SRCS}
           APPEND_STRING PROPERTY COMPILE_FLAGS " -mfpu=neon ")
add_definitions("-DHAVE_NEON=1")
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("[D] BUILD TYPE IS " ${CMAKE_BUILD_TYPE})
else()
    message("[R] BUILD TYPE IS " ${CMAKE_BUILD_TYPE})
    # -s选项为strip，不strip生成的库文件会很大
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s -Wl")
endif()
#endif()

set(PLATFORM_SOURCE
        #Platform/Android
        #${ENGINE_SOURCE_DIR}/Platform/Android/AndroidJNIMain.cpp
        	${ENGINE_SOURCE_DIR}/Application/Android/AndroidWindow.cpp
		${ENGINE_SOURCE_DIR}/Application/Android/AndroidApplication.cpp
		${ENGINE_SOURCE_DIR}/Vulkan/Android/VulkanAndroidPlatform.cpp
        		${ENGINE_SOURCE_DIR}/GenericPlatform/Android/AndroidPlatformTime.cpp
		${ENGINE_SOURCE_DIR}/GenericPlatform/Android/InputManagerAndroid.cpp
        )

set(PLATFORM_INCLUDES
        #${ENGINE_SOURCE_DIR}/Platform
)

if(ENGINE_DEBUG)

#Physx4.1使用DEBUG库需要声明-DNDEBUG
#定义后每个GL CALL会进行ERROR检查
    list(INSERT PLATFORM_DEFINITIONS 0 "DEBUG_OPENGL" "NDEBUG")
endif(ENGINE_DEBUG)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib
        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.
INCLUDE_DIRECTORIES(${ANDROID_NDK}/sources/android/cpufeatures)

add_library(cpu-features
        STATIC
        ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
add_library(app-glue
        STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

#TODO modify to static lib
add_library(libAssimp SHARED IMPORTED)
        set_target_properties(libAssimp PROPERTIES IMPORTED_LOCATION ${ENGINE_THIRDPARTY_DIR}/Assimp/lib/${ENGINE_ABI}/libassimp.so)

add_library(libIrrXML STATIC IMPORTED)
        set_target_properties(libIrrXML PROPERTIES IMPORTED_LOCATION ${ENGINE_THIRDPARTY_DIR}/Assimp/lib/${ENGINE_ABI}/libIrrXML.a)

list(APPEND SHARED_LIBS
libAssimp
libIrrXML
)


#Xinerama is an extension to the X Window System that enables X applications and window managers to use two or more physical displays as one large virtual display.
#xrandr is an official configuration utility to the RandR (Resize and Rotate) X Window System extension.
#Xxf86vm X11 XFree86 video mode extension library
#Xcursor is a simple library designed to help locate and load cursors.
#GLFW is an Open Source, multi-platform library for OpenGL, OpenGL ES and Vulkan development on the desktop.
#idn2 - Libidn2 Internationalized Domain Names (IDNA2008) conversion
set(PLATFORM_LINK_LIBS
    ${log-lib}
    GLESv3
    EGL
    m
    android
    jnigraphics
    app-glue
    cpu-features
   )

set(ENGINE_LINK_LIBS
    ${PLATFORM_LINK_LIBS}
    ${SHARED_LIBS}
   )

#NOTE: The order is very important, or report static lib link error(e.g., undefined sympol xxx)
#NEED FOLLOW THE ORDER BELOWING! ref: https://github.com/NVIDIAGameWorks/PhysX/issues/92
list(APPEND ENGINE_LINK_LIBS)

message(">>>>>>>>> Platform link libs: " ${PLATFORM_LINK_LIBS})
message(">>>>>>>>> Engine link libs: " ${ENGINE_LINK_LIBS})
