# @file CMakeLists.txt
# 
# CMake files for OS - Lab 3  (Simple File System)

# Can be run by any version of CMake >= this
cmake_minimum_required(VERSION 3.10.2)

# project info
project(filesys)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)

# Source files 
set(SOURCE filesys.c)

# convert application
add_executable(filesys ${SOURCE})

# Turn on warnings
if (MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # standard warnings
    add_compile_options(-Wall)
endif()

# run command
add_custom_target(run
        COMMENT "Run"
        COMMAND $<TARGET_FILE:filesys>
        DEPENDS filesys
        USES_TERMINAL
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)