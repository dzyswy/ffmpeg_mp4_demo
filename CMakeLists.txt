cmake_minimum_required(VERSION 3.8)
project(ffmpeg-demo)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(ENV{LD_LIBRARY_PATH} "/home/wy/src/ffmpeg/out/lib:$ENV{LD_LIBRARY_PATH}")
set(ENV{PKG_CONFIG_PATH} "/home/wy/src/ffmpeg/out/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")


add_compile_options(-fpermissive)
add_definitions(-D__STDC_CONSTANT_MACROS)

find_package (glog REQUIRED)

include_directories(./)

# 引入pkg-config模块
find_package(PkgConfig REQUIRED)

# 使用pkg-config查找FFmpeg库
pkg_check_modules(FFmpeg REQUIRED libavcodec libavformat libavutil libswscale libavdevice libavfilter libswresample)

# 包含FFmpeg的头文件路径
include_directories(${FFmpeg_INCLUDE_DIRS})

message(STATUS "find ffmpeg include: ${FFmpeg_INCLUDE_DIRS}")
message(STATUS "find ffmpeg lib: ${FFmpeg_LIBRARY_DIRS} ${FFmpeg_LIBRARIES}")

link_directories(${FFmpeg_LIBRARY_DIRS})

set(LIBS ${LIBS} glog::glog)
set(LIBS ${LIBS} ${FFmpeg_LIBRARIES})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/libs)


link_directories(${PROJECT_BINARY_DIR}/libs)


# add_executable(main  ./test/decode_video.cpp)
# target_link_libraries(main glog::glog)
# target_link_libraries(main ${FFmpeg_LIBRARIES})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)










