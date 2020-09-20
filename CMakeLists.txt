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
    )

set(LIB_DYL
    svher
    pthread
    yaml-cpp)

add_library(svher SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(svher)

add_executable(test tests/test.cpp)
add_dependencies(test svher)
force_redefine_file_macro_for_sources(test)
target_link_libraries(test ${LIB_DYL})

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config svher)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIB_DYL})

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread svher)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIB_DYL})

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util svher)
force_redefine_file_macro_for_sources(test_util)
target_link_libraries(test_util ${LIB_DYL})

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber svher)
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIB_DYL})

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler svher)
force_redefine_file_macro_for_sources(test_scheduler)
target_link_libraries(test_scheduler ${LIB_DYL})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)