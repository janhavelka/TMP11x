cmake_minimum_required(VERSION 3.16)

set(TMP11X_SOURCES src/Version.cpp)

if(COMMAND idf_component_register)
  idf_component_register(SRCS ${TMP11X_SOURCES} INCLUDE_DIRS include)
  target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
  return()
endif()

project(TMP11x LANGUAGES CXX)

add_library(TMP11x STATIC ${TMP11X_SOURCES})
target_include_directories(TMP11x PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(TMP11x PUBLIC cxx_std_17)

option(TMP11X_BUILD_TESTS "Build and register TMP11x host tests" OFF)
if(TMP11X_BUILD_TESTS)
  enable_testing()
  find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

  add_executable(tmp11x_version_test test/test_version.cpp)
  target_link_libraries(tmp11x_version_test PRIVATE TMP11x)
  add_test(NAME version_cpp COMMAND tmp11x_version_test)

  add_test(NAME version_generator_check
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_version.py" check)
  add_test(NAME version_sync
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_version_sync.py")
  add_test(NAME version_generator_regressions
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_generate_version.py")
  add_test(NAME register_ledger
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_register_ledger.py")
  add_test(NAME register_ledger_regressions
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_check_register_ledger.py")
endif()
