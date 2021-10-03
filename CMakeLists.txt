cmake_minimum_required(VERSION 3.1)
project(lime LANGUAGES CXX VERSION 1.0)

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------
option(LIME_BUILD_TESTS "Run tests" OFF)

# --------------------------------------------------------------------------------------------------------
# Create library
# --------------------------------------------------------------------------------------------------------
if (NOT MSVC)
    set(LIME_COMPILE_OPTIONS -Wall -Werror -Wextra -pedantic)
endif()

if (UNIX)
    file(GLOB src "src/disasm/disasm.cpp" "src/detour.cpp" "src/signature.cpp" "src/address.cpp" "src/*.linux.cpp")
elseif (WIN32)
    file(GLOB src "src/disasm/disasm.cpp" "src/detour.cpp" "src/signature.cpp" "src/address.cpp" "src/*.windows.cpp")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(${PROJECT_NAME} STATIC ${src})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME} PRIVATE ${LIME_COMPILE_OPTIONS})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------------------------------------------------------------
# Include "include" folder
# --------------------------------------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PUBLIC "include")

# --------------------------------------------------------------------------------------------------------
# Setup Catch2
# --------------------------------------------------------------------------------------------------------
if (LIME_BUILD_TESTS)
    message(STATUS "Building tests")
    find_package(Catch2 CONFIG REQUIRED)

    file(GLOB test_src "tests/*.cpp")
    add_executable(tests ${test_src})
    target_link_libraries(tests PRIVATE Catch2::Catch2 Catch2::Catch2WithMain ${PROJECT_NAME})
    
    include(CTest)
    include(Catch)
    catch_discover_tests(tests)

    target_compile_features(tests PRIVATE cxx_std_17)
    set_target_properties(tests PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
endif()

# --------------------------------------------------------------------------------------------------------
# Link required libraries
# --------------------------------------------------------------------------------------------------------
option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)

add_subdirectory("lib/zydis")

if (UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Zydis)