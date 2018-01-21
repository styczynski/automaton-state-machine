#
# FiniteAutomaton
#
#    This project was built as a part of the studies on Warsaw Univeristy
#
# @author Piotr Styczyński <piotrsty1@gmail.com>
# @copyright MIT
# @date 2018-01-21
#
cmake_minimum_required(VERSION 2.8.11)
project(FiniteAutomaton C)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif ()

# We do not want to see make calling things
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Compilation flags
set(CMAKE_C_FLAGS "-std=c11 -Wall -Wextra -pedantic")

#
# Compilation flags:
#
#   Use -DNO_LOG      to disable logging at all (only fatals are logged)
#   Use -DENABLE_GC   to enable GC
#
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wno-strict-overflow -DENABLE_GC -fno-exceptions")
set(CMAKE_C_FLAGS_DEBUG "-g -Wno-strict-overflow -DDEBUG -DENABLE_GC -O0")

# Inclusion directories
include_directories(./src/)
include_directories(./)

# Sources
file(GLOB SRC_FILES_VALIDATOR        ./src/validator.c)
file(GLOB SRC_FILES_TESTER           ./src/tester.c)
file(GLOB SRC_FILES_RUN              ./src/run.c)
file(GLOB SRC_FILES_AUTOVALIDATOR    ./src/autovalidator.c)

# Specify source files
find_package (Threads)

# ./validator - the server app
add_executable(validator ${SRC_FILES_VALIDATOR})
target_link_libraries(validator ${CMAKE_THREAD_LIBS_INIT} rt)

# ./tester - the client app
add_executable(tester ${SRC_FILES_TESTER})
target_link_libraries(tester ${CMAKE_THREAD_LIBS_INIT} rt)

# ./run - the server worker app
add_executable(run ${SRC_FILES_RUN})
target_link_libraries(run ${CMAKE_THREAD_LIBS_INIT} rt)

# ./autovalidator - autospawn for server and clients
add_executable(autovalidator ${SRC_FILES_AUTOVALIDATOR})
target_link_libraries(autovalidator ${CMAKE_THREAD_LIBS_INIT} rt)