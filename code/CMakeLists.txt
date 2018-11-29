cmake_minimum_required(VERSION 3.9)
project(main VERSION 1.21.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BILSMTREE_SOURCE_DIR "src")

add_subdirectory(${BILSMTREE_SOURCE_DIR})

add_executable(run "test.cc")

target_link_libraries(run bilsmtree)