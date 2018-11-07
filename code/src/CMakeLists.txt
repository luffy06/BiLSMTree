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
    "${PROJECT_SOURCE_DIR}/filesystem.cc"
    "${PROJECT_SOURCE_DIR}/hash.cc"
    "${PROJECT_SOURCE_DIR}/filter.cc"
    "${PROJECT_SOURCE_DIR}/block.cc"
    "${PROJECT_SOURCE_DIR}/table.cc"
    "${PROJECT_SOURCE_DIR}/visitfrequency.cc"
    "${PROJECT_SOURCE_DIR}/lsmtree.cc"
    "${PROJECT_SOURCE_DIR}/logmananger.cc"
    "${PROJECT_SOURCE_DIR}/kvserver.cc"
    "${PROJECT_SOURCE_DIR}/skiplist.cc"
    "${PROJECT_SOURCE_DIR}/bilist.cc"
    "${PROJECT_SOURCE_DIR}/lru2q.cc"
    "${PROJECT_SOURCE_DIR}/cacherserver.cc"
    "${PROJECT_SOURCE_DIR}/db.cc"

  $<$<VERSION_GREATER:CMAKE_VERSION,3.2>:PUBLIC>
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/flash.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/filesystem.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/slice.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/hash.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/filter.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/block.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/table.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/visitfrequency.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/lsmtree.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/logmananger.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/kvserver.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/skiplist.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/bilist.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/lru2q.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/cacherserver.h"
    "${BILSMTREE_PUBLIC_INCLUDE_DIR}/db.h"
)

target_include_directories(bilsmtree
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
