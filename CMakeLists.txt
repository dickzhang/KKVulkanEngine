# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。

cmake_minimum_required (VERSION 3.8)
project ("KKVulkanEngine")

# 包含子项目。
add_subdirectory ("Engine")

if( ${NS_DEBUG_PROJECT} AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
#调试项目，需要在CMakeSettings.json中改为-DNS_DEBUG_PROJECT=ON，注意RELEASE下自动关闭项目调试，防止项目库被错误发布
#add_subdirectory ("DebugProjects/NewProject")
endif()