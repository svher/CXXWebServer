cmake_minimum_required(VERSION 3.16)
project(webserver)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "-rdynamic -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-builtin-macro-redefined")

include_directories(svher)
include_directories(/usr/include)

set(LIB_SRC
    svher/log.cpp
    svher/util.cpp
    svher/config.cpp
    svher/thread.cpp
    svher/fiber.cpp
    svher/scheduler.cpp
    svher/iomanager.cpp
    )

set(LIB_DYL
    webserver
    pthread
    yaml-cpp)

add_library(webserver SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(webserver)

# format: target src depends link_libraries
my_add_executable(test_scheduler "tests/test_scheduler.cpp" webserver "${LIB_DYL}")
my_add_executable(test_fiber "tests/test_fiber.cpp" webserver "${LIB_DYL}")
my_add_executable(test_iomanager "tests/test_iomanager.cpp" webserver "${LIB_DYL}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)