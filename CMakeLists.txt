cmake_minimum_required(VERSION 3.16)
project(webserver)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXX_FLAGS} -rdynamic -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-builtin-macro-redefined")
set(CMAKE_CXX_STANDARD 14)

# 注意这里不要 include_directories(svher)，不然如果自己的文件和库文件重名，会导致 include 错误 !!!
include_directories(.)
include_directories(/usr/include)

set(LIB_SRC
    svher/log.cpp
    svher/util.cpp
    svher/config.cpp
    svher/thread.cpp
    svher/fiber.cpp
    svher/scheduler.cpp
    svher/iomanager.cpp
    svher/timer.cpp
    svher/hook.cpp
    svher/fdmanager.cpp
    svher/address.cpp
    svher/socket.cpp
    svher/bytearray.cpp
    )

set(LIB_DYL
    webserver
    pthread
    yaml-cpp
    dl
    )

add_library(webserver SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(webserver)

# format: target src depends link_libraries
my_add_executable(test_scheduler "tests/test_scheduler.cpp" webserver "${LIB_DYL}")
my_add_executable(test_fiber "tests/test_fiber.cpp" webserver "${LIB_DYL}")
my_add_executable(test_iomanager "tests/test_iomanager.cpp" webserver "${LIB_DYL}")
my_add_executable(test_hook "tests/test_hook.cpp" webserver "${LIB_DYL}")
my_add_executable(test_address "tests/test_address.cpp" webserver "${LIB_DYL}")
my_add_executable(test_socket "tests/test_socket.cpp" webserver "${LIB_DYL}")

my_add_executable(test_bytearray "tests/test_bytearray.cpp" webserver "${LIB_DYL}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)