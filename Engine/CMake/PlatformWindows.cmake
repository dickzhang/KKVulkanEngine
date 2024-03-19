#set platform related cmake definition and config

message(">>>>>>>>>>>> WINDOWS PLATFORM ENGINE SOURCE: " ${ENGINE_SOURCE_DIR})
message(">>>>>>>>>>>> WINDOWS PLATFORM ENGINE THIRDPARTY: " ${ENGINE_THIRDPARTY_DIR})

set(PLATFORM_CXX_FLAGS "/MP /permissive- /DCURL_STATICLIB /DBUILDING_LIBCURL  /DFREEIMAGE_LIB /DTINYXML2_EXPORT /DHTTP_ONLY /D_WINDOWS /DENGINE_EXPORTS /D_CRT_SECURE_NO_WARNINGS /std:c++17")
#set(PLATFORM_CXX_FLAGS "${PLATFORM_CXX_FLAGS} /Gy /Oi /openmp /sdl- /DFREEIMAGE_LIB /Zc:twoPhase- /utf-8 /Zc:wchar_t /WX-")
set(PLATFORM_CXX_FLAGS "${PLATFORM_CXX_FLAGS} /Gd /Gy /Oi /openmp /sdl- /Zc:twoPhase- /utf-8 /Zc:wchar_t /WX-")
#/FORCE:MULTIPLE to solve re-define symbol in WebP and FreeImageLib
set(PLATFORM_LINK_FLAGS "/OPT:ICF /INCREMENTAL:NO /OPT:NOREF /FORCE:MULTIPLE /NODEFAULTLIB:MSVCRTD /SUBSYSTEM:WINDOWS")
#set(PLATFORM_LINK_FLAGS "/OPT:ICF /SUBSYSTEM:WINDOWS")

set(PLATFORM_SOURCE 
        #Platform/Windows
        ${ENGINE_SOURCE_DIR}/Application/Windows/WinWindow.cpp
		${ENGINE_SOURCE_DIR}/Application/Windows/WinApplication.cpp
		${ENGINE_SOURCE_DIR}/Vulkan/Windows/VulkanWindowsPlatform.cpp
        ${ENGINE_SOURCE_DIR}/GenericPlatform/Windows/WindowsPlatformTime.cpp
		${ENGINE_SOURCE_DIR}/GenericPlatform/Windows/InputManagerWindows.cpp
        ${ENGINE_SOURCE_DIR}/Launch/LaunchWindows.cpp
        )

 set(PLATFORM_INCLUDES 
      ${ENGINE_THIRDPARTY_DIR}/vulkan/windows/include
 )

if(ENGINE_DEBUG)
    list(INSERT PLATFORM_DEFINITIONS 0 "/DDEBUG_OPENGL" "/D_DEBUG")
endif(ENGINE_DEBUG)

set(ENGINE_LINK_LIBS 
   )

list(APPEND ENGINE_LINK_LIBS )

message(">>>>>>>>> Platform link libs: " ${PLATFORM_LINK_LIBS})
message(">>>>>>>>> Engine link libs: " ${ENGINE_LINK_LIBS})
