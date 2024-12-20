# set cross-compiled system type, it's better not use the type which cmake cannot recognized.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
# when hislicon SDK was installed, toolchain was installed in the path as below: 
set(CMAKE_C_COMPILER "aarch64-mix210-linux-gcc")
set(CMAKE_CXX_COMPILER "aarch64-mix210-linux-g++")
set(CMAKE_FIND_ROOT_PATH "/home/wy/EulerPi/toolchain/aarch64-mix210-linux /home/wy/tftpboot/aarch64-linux-gnu")
 
set(glog_DIR /home/wy/tftpboot/aarch64-linux-gnu/lib/cmake/glog)
set(OpenCV_DIR /home/wy/tftpboot/aarch64-linux-gnu/lib/cmake/opencv4)
set(ENV{PKG_CONFIG_PATH} "/home/wy/tftpboot/aarch64-linux-gnu/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
set(ENV{LD_LIBRARY_PATH} "/home/wy/tftpboot/aarch64-linux-gnu/lib:$ENV{LD_LIBRARY_PATH}")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,/home/wy/tftpboot/aarch64-linux-gnu/lib")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath-link,/home/wy/tftpboot/aarch64-linux-gnu/lib")

# set searching rules for cross-compiler
if(NOT CMAKE_FIND_ROOT_PATH_MODE_PROGRAM)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_LIBRARY)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_INCLUDE)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_PACKAGE)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

# set ${CMAKE_C_FLAGS} and ${CMAKE_CXX_FLAGS}flag for cross-compiled process
#set(CMAKE_CXX_FLAGS "-march=armv7-a -mfloat-abi=softfp -mfpu=neon ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# cache flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "c flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "c++ flags")

