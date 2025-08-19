cmake_minimum_required(VERSION 3.10)

project(SimpleCppWebServer LANGUAGES CXX)

# Set C++ standard and compiler warnings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Source files
add_executable(server
    src/main.cpp
)

# Platform-specific settings
if(WIN32)
    message(STATUS "Configuring for Windows...")
    target_compile_definitions(server PRIVATE _WIN32_WINNT=0x0601)
    target_link_libraries(server ws2_32)
else()
    message(STATUS "Configuring for Linux/Unix...")
    find_package(Threads REQUIRED)
    target_link_libraries(server PRIVATE Threads::Threads)
endif()

# Make sure the www directory is copied to build directory for testing
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/www DESTINATION ${CMAKE_CURRENT_BINARY_DIR})