cmake_minimum_required(VERSION 3.1)
project(lime LANGUAGES CXX VERSION 1.0)

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

option(LIME_BUILD_TESTS "Run tests" ON)
option(LIME_IS_CI       "Determines if lime is built from the GitHub CI" OFF)

# --------------------------------------------------------------------------------------------------------
# Create library
# --------------------------------------------------------------------------------------------------------

if (UNIX)
    file(GLOB src "src/disasm/disasm.cpp" "src/detour.cpp" "src/signature.cpp" "src/address.cpp" "src/*.linux.cpp")
elseif (WIN32)
    file(GLOB src "src/disasm/disasm.cpp" "src/detour.cpp" "src/signature.cpp" "src/address.cpp" "src/*.windows.cpp")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(${PROJECT_NAME} STATIC ${src})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wextra -pedantic)
endif()

# --------------------------------------------------------------------------------------------------------
# Include "include" folder
# --------------------------------------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PUBLIC "include")

# --------------------------------------------------------------------------------------------------------
# Setup Catch2
# --------------------------------------------------------------------------------------------------------

if (LIME_BUILD_TESTS)
    message(STATUS "Building tests")

    if (NOT TARGET Catch2::Catch2WithMain)
        include(FetchContent)
        FetchContent_Declare(Catch2 GIT_REPOSITORY "https://github.com/catchorg/Catch2" GIT_TAG v2.13.7)
        option(CATCH_BUILD_STATIC_LIBRARY "" ON)
        FetchContent_MakeAvailable(Catch2)
    endif()

    file(GLOB test_src "tests/*.cpp")
    
    if (LIME_IS_CI)
        list(FILTER test_src EXCLUDE REGEX ".*keyboard.*")
        message(STATUS "Building from CI, disabling keyboard tests")
    endif()

    if (MSVC)
        set_source_files_properties("tests/detour.test.cpp" PROPERTIES COMPILE_FLAGS /Od)
        set_source_files_properties("tests/address.test.cpp" PROPERTIES COMPILE_FLAGS /Od)
    else()
        set_source_files_properties("tests/detour.test.cpp" PROPERTIES COMPILE_FLAGS -O0)
    endif()

    add_executable(tests ${test_src})
    target_link_libraries(tests PRIVATE Catch2::Catch2 Catch2::Catch2WithMain ${PROJECT_NAME})

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
    find_package(X11 REQUIRED)
    target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE dl ${X11_LIBRARIES} ${X11_XTest_LIB})
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Imagehlp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Zydis)

# --------------------------------------------------------------------------------------------------------
# Install Targets
# --------------------------------------------------------------------------------------------------------

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib INCLUDES DESTINATION include)