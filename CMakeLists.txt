cmake_minimum_required(VERSION 3.16)
project(lime LANGUAGES CXX VERSION 2.0)

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

option(lime_tests                      "Compile tests"                                      OFF)
set(lime_entrypoint "OFF" CACHE STRING "Wether or not to enable the cross-platform entry-point")

# --------------------------------------------------------------------------------------------------------
# Ensure valid entrypoint options
# --------------------------------------------------------------------------------------------------------

set(lime_entrypoints OFF ON static native)
set_property(CACHE lime_entrypoint PROPERTY STRINGS ${lime_entrypoints})

if (NOT lime_entrypoint IN_LIST lime_entrypoints)
    message(FATAL_ERROR "Bad Entrypoint, expected one of ${lime_entrypoints}")
endif()

if (lime_entrypoint STREQUAL "ON")
  set(lime_entrypoint "native")
  message(STATUS "[lime] Entrypoint was set to 'ON', using 'native' as default")
endif()

# --------------------------------------------------------------------------------------------------------
# CMake options
# --------------------------------------------------------------------------------------------------------

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --------------------------------------------------------------------------------------------------------
# Setup library
# --------------------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} STATIC)
add_library(cr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (UNIX AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -pedantic -pedantic-errors -Wfatal-errors)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup includes
# --------------------------------------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PUBLIC  "include")
target_include_directories(${PROJECT_NAME} PRIVATE "private" "include/lime")

# --------------------------------------------------------------------------------------------------------
# Setup sources
# --------------------------------------------------------------------------------------------------------

target_sources(${PROJECT_NAME} PRIVATE 
  "src/disasm.cpp"
  "src/address.cpp"
  "src/hooks.hook.cpp"
  "src/instruction.cpp"
  "src/utils.signature.cpp"
)

# --------------------------------------------------------------------------------------------------------
# Setup required libraries
# --------------------------------------------------------------------------------------------------------

include("cmake/cpm.cmake")

CPMAddPackage(
  NAME           Zydis
  VERSION        4.0.0
  GIT_REPOSITORY "https://github.com/zyantific/zydis"
  OPTIONS        "ZYDIS_BUILD_TOOLS OFF" "ZYDIS_BUILD_DOXYGEN OFF" "ZYDIS_BUILD_EXAMPLES OFF"
)

CPMAddPackage(
  NAME           flags
  VERSION        1.0
  GIT_REPOSITORY "https://github.com/Curve/flags"
)

CPMAddPackage(
  NAME           expected
  GIT_TAG        3f0ca7b
  GIT_REPOSITORY "https://github.com/TartanLlama/expected"
  OPTIONS        "EXPECTED_BUILD_TESTS OFF"
)

CPMAddPackage(
  NAME           boost_callable_traits
  GIT_TAG        2.3.3
  GIT_REPOSITORY "https://github.com/boostorg/callable_traits"
)

target_link_libraries(${PROJECT_NAME} PUBLIC  cr::flags tl::expected boost_callable_traits)
target_link_libraries(${PROJECT_NAME} PRIVATE Zydis)

# --------------------------------------------------------------------------------------------------------
# Setup backends
# --------------------------------------------------------------------------------------------------------

if (UNIX AND NOT MINGW)
  file(GLOB src "src/*.linux*.cpp")
  list(FILTER src EXCLUDE REGEX "entrypoint")
  target_sources(${PROJECT_NAME} PRIVATE ${src})
endif()

if (WIN32)
  file(GLOB src "src/*.win*.cpp")
  list(FILTER src EXCLUDE REGEX "entrypoint")
  target_sources(${PROJECT_NAME} PRIVATE ${src})
  target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
endif()

if (lime_entrypoint STREQUAL "native")
  if (WIN32)
    target_sources(${PROJECT_NAME} PRIVATE "src/entrypoint.win.cpp")
  else()
    target_sources(${PROJECT_NAME} PRIVATE "src/entrypoint.linux.cpp")
  endif()
endif()

if (lime_entrypoint STREQUAL "static")
  target_sources(${PROJECT_NAME} PRIVATE "src/entrypoint.static.cpp")
endif()

# --------------------------------------------------------------------------------------------------------
# Setup tests
# --------------------------------------------------------------------------------------------------------

if (lime_tests)
  CPMAddPackage(
    NAME           Catch2
    VERSION        3.3.2
    GIT_REPOSITORY "https://github.com/catchorg/Catch2"
  )

  file(GLOB src "tests/*.test.cpp")
  add_executable(${PROJECT_NAME}_tests ${src})

  target_compile_features(${PROJECT_NAME}_tests INTERFACE cxx_std_20)
  set_target_properties(${PROJECT_NAME}_tests PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

  target_link_libraries(${PROJECT_NAME}_tests PUBLIC Catch2WithMain ${PROJECT_NAME})
endif()