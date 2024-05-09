cmake_minimum_required(VERSION 3.17...3.23)
set(CMAKE_BUILD_TYPE "Release")

project(
    SPP_CONVOLUTION_ASSIGNMENT
    VERSION 1
    DESCRIPTION "An assignment on optimizing applying image convolutions"
    LANGUAGES CXX CUDA)

find_package(CUDAToolkit REQUIRED)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib) # Set libs dir to /build/lib

add_subdirectory(src)
add_subdirectory(tester)

