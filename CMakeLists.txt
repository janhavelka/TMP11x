cmake_minimum_required(VERSION 3.16)

set(TMP11X_SOURCES
  src/CommandTable.cpp
  src/Config.cpp
  src/Identity.cpp
  src/RegisterTransport.cpp
  src/TMP11x.cpp
  src/Version.cpp)

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

  add_executable(tmp11x_value_types_test
    test/test_value_types.cpp
    test/config_header.cpp
    test/status_header.cpp)
  target_link_libraries(tmp11x_value_types_test PRIVATE TMP11x)
  target_include_directories(tmp11x_value_types_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  add_test(NAME value_types_cpp COMMAND tmp11x_value_types_test)

  add_executable(tmp11x_register_transport_test
    test/test_command_table.cpp
    test/test_register_transport.cpp
    test/support/FakeTmp11xDevice.cpp)
  target_link_libraries(tmp11x_register_transport_test PRIVATE TMP11x)
  target_include_directories(tmp11x_register_transport_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  add_test(NAME register_transport_cpp COMMAND tmp11x_register_transport_test)

  add_executable(tmp11x_identity_test
    test/test_identity.cpp
    test/support/FakeTmp11xDevice.cpp)
  target_link_libraries(tmp11x_identity_test PRIVATE TMP11x)
  target_include_directories(tmp11x_identity_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  add_test(NAME identity_cpp COMMAND tmp11x_identity_test)

  add_executable(tmp11x_binding_test
    test/test_binding.cpp
    test/support/FakeTmp11xDevice.cpp)
  target_link_libraries(tmp11x_binding_test PRIVATE TMP11x)
  add_test(NAME binding_cpp COMMAND tmp11x_binding_test)

  foreach(target TMP11x tmp11x_value_types_test tmp11x_register_transport_test tmp11x_identity_test
                 tmp11x_binding_test)
    set_target_properties(${target} PROPERTIES
      CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    if(MSVC)
      target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
  endforeach()

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
