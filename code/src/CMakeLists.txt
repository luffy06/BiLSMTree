cmake_minimum_required(VERSION 3.9)
project(bilsmtree VERSION 1.0 LANGUAGES C CXX)

# This project can use C11, but will gracefully decay down to C89.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS OFF)

# This project requires C++11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BILSMTREE_PUBLIC_INCLUDE_DIR "include")

include_directories(
  "${PROJECT_BINARY_DIR}/include"
  "${PROJECT_SOURCE_DIR}"
)

add_library(bilsmtree "")
target_sources(bilsmtree
  PRIVATE
    "${PROJECT_SOURCE_DIR}/flash.cc"
  $<$<VERSION_GREATER:CMAKE_VERSION,3.2>:PUBLIC>
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/flash.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/filesystem.h"
)

target_include_directories(bilsmtree
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
