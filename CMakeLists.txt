cmake_minimum_required(VERSION 3.21)
project(lime LANGUAGES CXX VERSION 7.0)

# +-------------------------------------------------------------------------------------------------------+
# | Library options                                                                                       |
# +-------------------------------------------------------------------------------------------------------+

option(lime_tests               "Build tests"                                         OFF)
option(lime_no_alloc2           "Disables VirtualAlloc2"                              OFF)
option(lime_static_entrypoint   "Use platform independent entrypoint implementation"  OFF)

# +-------------------------------------------------------------------------------------------------------+
# | CMake options                                                                                         |
# +-------------------------------------------------------------------------------------------------------+

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# +-------------------------------------------------------------------------------------------------------+
# | Setup library                                                                                         |
# +-------------------------------------------------------------------------------------------------------+

add_library(${PROJECT_NAME} STATIC)
add_library(cr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (PROJECT_IS_TOP_LEVEL AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -pedantic -pedantic-errors -Wfatal-errors)
endif()

if (NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-warning-option -Wno-missing-field-initializers -Wno-cast-function-type)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(${PROJECT_NAME} PUBLIC -Wno-attributes=lime::)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(${PROJECT_NAME} PUBLIC /wd5030 /utf-8)
else()
  target_compile_options(${PROJECT_NAME} PUBLIC -Wno-unknown-attributes)
endif()

# +-------------------------------------------------------------------------------------------------------+
# | Setup includes                                                                                        |
# +-------------------------------------------------------------------------------------------------------+

target_include_directories(${PROJECT_NAME} PUBLIC  "include")
target_include_directories(${PROJECT_NAME} PRIVATE "private" "include/lime")

# +-------------------------------------------------------------------------------------------------------+
# | Setup sources                                                                                         |
# +-------------------------------------------------------------------------------------------------------+

target_sources(${PROJECT_NAME} PRIVATE
  "src/lib.cpp"
  "src/hook.cpp"
  "src/address.cpp"
  "src/signature.cpp"
  "src/instruction.cpp"
)

# +-------------------------------------------------------------------------------------------------------+
# | Setup cmake exports                                                                                   |
# +-------------------------------------------------------------------------------------------------------+

include("cmake/proxy.cmake")

# +-------------------------------------------------------------------------------------------------------+
# | Setup required libraries                                                                              |
# +-------------------------------------------------------------------------------------------------------+

include("cmake/cpm.cmake")

CPMAddPackage(
  NAME           Zydis
  VERSION        4.1.1
  GIT_REPOSITORY "https://github.com/zyantific/zydis"
)

CPMAddPackage(
  NAME           flagpp
  VERSION        3.1.0
  GIT_REPOSITORY "https://github.com/Curve/flagpp"
)

target_link_libraries(${PROJECT_NAME} PRIVATE Zydis)
target_link_libraries(${PROJECT_NAME} PUBLIC  cr::flagpp)

# +-------------------------------------------------------------------------------------------------------+
# | Setup backends                                                                                        |
# +-------------------------------------------------------------------------------------------------------+

if (UNIX AND NOT MINGW)
  CPMFindPackage(
    NAME           scnlib
    VERSION        4.0.1
    GIT_REPOSITORY "https://github.com/eliaskosunen/scnlib"
    SYSTEM         ON
  )

  file(GLOB src "src/*.linux*.cpp")
  target_link_libraries(${PROJECT_NAME} PRIVATE scn::scn)
endif()

if (WIN32)
  file(GLOB src "src/*.win*.cpp")
  target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
endif()

target_sources(${PROJECT_NAME} PRIVATE ${src})

# +-------------------------------------------------------------------------------------------------------+
# | Setup entrypoint                                                                                      |
# +-------------------------------------------------------------------------------------------------------+

# TODO: Entrypoint should be its own target

# if (lime_static_entrypoint)
#   target_sources(${PROJECT_NAME} PRIVATE "src/entrypoint.static.cpp")
# elseif (NOT lime_static_entrypoint AND WIN32)
#   target_sources(${PROJECT_NAME} PRIVATE "src/entrypoint.win.cpp")
# elseif(NOT lime_static_entrypoint)
#   target_sources(${PROJECT_NAME} PRIVATE "src/entrypoint.linux.cpp")
# endif()


if (lime_no_alloc2)
  target_compile_definitions(${PROJECT_NAME} PUBLIC LIME_DISABLE_ALLOC2)
endif()

# +-------------------------------------------------------------------------------------------------------+
# | Setup tests                                                                                           |
# +-------------------------------------------------------------------------------------------------------+

if (lime_tests)
  message(STATUS "[lime] Building tests")
  add_subdirectory(tests)
endif()
