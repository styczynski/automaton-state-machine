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
set(CMAKE_C_FLAGS_RELEASE "-O3 -DENABLE_GC -fno-exceptions")
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG -DENABLE_GC -O0")

# Inclusion directories
include_directories(./src/)
include_directories(./)

# Sources
file(GLOB SRC_FILES_VALIDATOR        ./src/validator.c)
file(GLOB SRC_FILES_TESTER           ./src/tester.c)
file(GLOB SRC_FILES_RUN              ./src/run.c)
file(GLOB SRC_FILES_AUTOVALIDATOR    ./src/autovalidator.c)

file(GLOB SRC_FILES_EX_PIPES ./examples/pipes.c)

# Specify source files
find_package (Threads)

add_executable(validator ${SRC_FILES_VALIDATOR})
target_link_libraries(validator ${CMAKE_THREAD_LIBS_INIT} rt)

add_executable(tester ${SRC_FILES_TESTER})
target_link_libraries(tester ${CMAKE_THREAD_LIBS_INIT} rt)

add_executable(run ${SRC_FILES_RUN})
target_link_libraries(run ${CMAKE_THREAD_LIBS_INIT} rt)

add_executable(pipes ${SRC_FILES_EX_PIPES})
target_link_libraries(pipes ${CMAKE_THREAD_LIBS_INIT} rt)

add_executable(autovalidator ${SRC_FILES_AUTOVALIDATOR})
target_link_libraries(autovalidator ${CMAKE_THREAD_LIBS_INIT} rt)