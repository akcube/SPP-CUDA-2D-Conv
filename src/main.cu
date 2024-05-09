#include <iostream>
#include <fstream>
#include <memory>
#include <cstdint>
#include <filesystem>
#include <string>

namespace solution{
        #define CUDA_ERROR_CHECK(ans) { cudaAssert((ans), __FILE__, __LINE__); } 
        inline void cudaAssert(cudaError_t code, const char *file, int line, bool abort=true){
                if (code != cudaSuccess){
                        fprintf(stderr,"CUDA Error: %s %s %d\n", cudaGetErrorString(code), file, line);
                        if (abort) exit(code);
                }
        }

        __global__ void convolution2D(){
                printf("Hello from the GPU\n");
        }

        std::string compute(const std::string &bitmap_path, const float kernel[3][3], const std::int32_t num_rows, const std::int32_t num_cols){
                std::string sol_path = std::filesystem::temp_directory_path() / "student_sol.bmp";
                std::ofstream sol_fs(sol_path, std::ios::binary);
                std::ifstream bitmap_fs(bitmap_path, std::ios::binary);
                const auto img = std::make_unique<float[]>(num_rows * num_cols);
                bitmap_fs.read(reinterpret_cast<char*>(img.get()), sizeof(float) * num_rows * num_cols);
                // Do some allocations etc.
                // Call CUDA Kernel
                convolution2D<<<1, 1>>>();
                bitmap_fs.close();
                return sol_path;
        }
};
