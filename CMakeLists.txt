cmake_minimum_required(VERSION 3.16)
project(WebServer)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "-rdynamic -g -std=c++11 -Wall -Wno-deprecated -Werror")

include_directories(svher)
include_directories(/usr/include)

set(LIB_SRC
    svher/log.cpp
    svher/util.cpp
    svher/config.cpp
    )

add_library(svher SHARED ${LIB_SRC})

add_executable(test tests/test.cpp)
add_dependencies(test svher)
target_link_libraries(test svher)

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config svher)
target_link_libraries(test_config svher yaml-cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)