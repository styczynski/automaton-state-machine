cmake_minimum_required(VERSION 2.8.11)
project(BMatchingApprox C)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    #set(CMAKE_BUILD_TYPE "Release")
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

# We do want to see make calling things
#set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Compilation flags
set(CMAKE_C_FLAGS "-std=c11 -Wall -Wextra -pedantic")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNO_LOG -fno-exceptions")
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG -O0")

# Inclusion directories
include_directories(./src/)
include_directories(./)

# Sources
file(GLOB SRC_FILES_VALIDATOR ./src/validator.c)
file(GLOB SRC_FILES_TESTER    ./src/tester.c)
file(GLOB SRC_FILES_RUN       ./src/run.c)

# Specify source files
find_package (Threads)

add_executable(validator ${SRC_FILES_VALIDATOR})
target_link_libraries(validator ${CMAKE_THREAD_LIBS_INIT} rt)

add_executable(tester ${SRC_FILES_TESTER})
target_link_libraries(tester ${CMAKE_THREAD_LIBS_INIT} rt)

add_executable(run ${SRC_FILES_RUN})
target_link_libraries(run ${CMAKE_THREAD_LIBS_INIT} rt)
