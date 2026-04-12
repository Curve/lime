cmake_minimum_required(VERSION 3.21)
project(lime LANGUAGES CXX VERSION 8.0.0)

# +-------------------------------------------------------------------------------------------------------+
# | Library switches                                                                                      |
# +-------------------------------------------------------------------------------------------------------+

option(lime_tests "Build tests" OFF)

# +-------------------------------------------------------------------------------------------------------+
# | Library options                                                                                       |
# +-------------------------------------------------------------------------------------------------------+

set(lime_entrypoint "Disabled" CACHE STRING "The cross-platform entrypoint implementation to use")

# +-------------------------------------------------------------------------------------------------------+
# | CMake options                                                                                         |
# +-------------------------------------------------------------------------------------------------------+

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# +-------------------------------------------------------------------------------------------------------+
# | Convenience wrapper over `message`                                                                    |
# +-------------------------------------------------------------------------------------------------------+

function(lime_message LEVEL MESSAGE)
  message(${LEVEL} "lime: ${MESSAGE}")
endfunction()

# +-------------------------------------------------------------------------------------------------------+
# | Ensure valid library options                                                                          |
# +-------------------------------------------------------------------------------------------------------+

set(lime_valid_entrypoints Disabled Platform Static)
set_property(CACHE lime_entrypoint PROPERTY STRINGS ${lime_valid_entrypoints})

if (NOT lime_entrypoint IN_LIST lime_valid_entrypoints)
  lime_message(FATAL_ERROR "Bad Entrypoint, expected one of ${lime_valid_entrypoints}")
endif()

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
  set(sources
    "src/lib.linux.cpp"
    "src/page.linux.cpp"
  )

  if (lime_entrypoint STREQUAL "Platform")
    list(APPEND sources
      "src/entrypoint.linux.cpp"
    )
  endif()

  target_sources(${PROJECT_NAME} PRIVATE ${sources})
endif()

if (WIN32)
  set(sources
    "src/lib.win.cpp"
    "src/page.win.cpp"
  )

  if (lime_entrypoint STREQUAL "Platform")
    list(APPEND sources
      "src/entrypoint.win.cpp"
    )
  endif()

  target_sources(${PROJECT_NAME} PRIVATE ${sources})
  target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
endif()

if (lime_entrypoint STREQUAL "Static")
  target_sources(${PROJECT_NAME} PRIVATE
    "src/entrypoint.cpp"
  )
endif()

# +-------------------------------------------------------------------------------------------------------+
# | Setup tests                                                                                           |
# +-------------------------------------------------------------------------------------------------------+

if (lime_tests)
  lime_message(STATUS "Building tests")
  add_subdirectory(tests)
endif()
