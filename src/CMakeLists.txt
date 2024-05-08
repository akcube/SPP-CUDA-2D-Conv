add_library(studentlib main.cpp)
target_include_directories(studentlib PUBLIC ${CMAKE_SOURCE_DIR}/include)
find_package(CUDAToolkit REQUIRED)
set_target_properties(studentlib PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
target_compile_options(studentlib PRIVATE -Ofast -march=native)

