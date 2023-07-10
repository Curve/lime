cmake_minimum_required(VERSION 3.16)
project(lime LANGUAGES CXX VERSION 2.0)

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

option(lime_tests       "Compile tests"                                           OFF)
option(lime_entrypoint  "Wether or not to enable the cross-platform entry-point"  OFF)

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
  NAME           fmt
  GIT_TAG        10.0.0        
  GIT_REPOSITORY "https://github.com/fmtlib/fmt"
)

CPMAddPackage(
  NAME           Zydis
  GIT_TAG        a6d0c71
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
)

CPMAddPackage(
  NAME           boost_callable_traits
  GIT_TAG        2.3.3
  GIT_REPOSITORY "https://github.com/boostorg/callable_traits"
)

target_link_libraries(${PROJECT_NAME} PRIVATE cr::flags tl::expected fmt Zydis)

# --------------------------------------------------------------------------------------------------------
# Setup backends
# --------------------------------------------------------------------------------------------------------

if (UNIX)
  file(GLOB src "src/*.linux*.cpp")
  
  if (NOT lime_entrypoint)
    list(FILTER src EXCLUDE REGEX "entrypoint")
  endif()

  target_sources(${PROJECT_NAME} PRIVATE ${src})
endif()