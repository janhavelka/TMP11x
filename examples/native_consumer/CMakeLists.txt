cmake_minimum_required(VERSION 3.16)
project(tmp11x_native_consumer LANGUAGES CXX)

set(TMP11X_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH
    "Path to the TMP11x source package")
set(TMP11X_BUILD_TESTS OFF)
add_subdirectory("${TMP11X_SOURCE_DIR}" tmp11x)

add_executable(tmp11x_native_consumer main.cpp)
target_link_libraries(tmp11x_native_consumer PRIVATE TMP11x)
