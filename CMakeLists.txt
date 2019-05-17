cmake_minimum_required(VERSION 3.0)

project(blaze-protorpc)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CXX_FLAGS
        -g
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wold-style-cast
        -Wsequence-point
        -Woverloaded-virtual
        -Wpointer-arith
        -Wshadow
        -Wthread-safety
        -Wwrite-strings
        -march=native
        -std=c++17
        -Wno-unused-parameter
        -Wno-null-dereference
        -Wno-sign-conversion
        -Wno-unused-private-field
        -fmodules)

if(CMAKE_BUILD_BITS EQUAL 32)
    list(APPEND CXX_FLAGS "-m32")
endif()

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/third_party/include)
link_directories(${PROJECT_SOURCE_DIR}/third_party/lib)

find_package(Boost REQUIRED)
find_package(Protobuf)
find_package(ZLIB)
find_package(Threads)

if (BOOST_FOUND)
    message(STATUS "found Boost")
endif()

if (PROTOBUF_FOUND)
    message(STATUS "found Protobuf")
endif()

if (ZLIB_FOUND)
    message(STATUS "found zlib")
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(examples)
add_subdirectory(protorpc)


add_executable(main_test main.cc protorpc/rpc.pb.cc)
target_link_libraries(main_test protobuf pthread)