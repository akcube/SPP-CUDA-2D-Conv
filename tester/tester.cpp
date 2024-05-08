#include <iostream>
#include <functional>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <exception>
#include <memory>
#include <fstream>
#include <immintrin.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <studentlib.h>

void __terminate_gracefully(const std::string &msg) noexcept {
	std::cout << -1 << std::endl;
	std::cerr << msg << std::endl;
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]){
	#if defined(__cpp_lib_filesystem)
		// Header available
	#else
		__terminate_gracefully("<filesystem> header is not supported by this compiler");
	#endif
	try{
		// Parse arguments
		if(argc < 3) __terminate_gracefully("Usage: ./tester.out <num-image-rows> <num-image-cols> <optional:seed>");	
		std::random_device rd;
		std::mt19937 rng(argc > 3 ? std::atoi(argv[3]) : rd());
		std::int32_t num_rows = std::atoi(argv[1]), num_cols = std::atoi(argv[2]);
		// Util func
		std::function<float(void)> generateRandomfloat = [&](){
			static std::uniform_real_distribution<float> distribution(0.0, 255.0);
			return distribution(rng);
		};
		// Create psuedo-bitmap file
		std::string id = std::to_string(num_rows) + "x" + std::to_string(num_cols);
		std::string bitmap_path = std::filesystem::temp_directory_path() / ("fake-bitmap-" + id + ".bmp");
		std::cout << "[1/6] Looking for input file " << bitmap_path << std::endl;
		const auto img = std::make_unique<float[]>(num_rows * num_cols);
		if(std::filesystem::exists(bitmap_path)) std::cout << "[2/6] Input file found, using existing input file" << std::endl;
		else{
			std::cout << "[2/6] Input file not found. Creating new test data" << std::endl;
			std::ofstream bitmap_fs(bitmap_path, std::ios::binary);
			for(std::int32_t i=0; i < num_rows * num_cols; i++) {
				img[i] = generateRandomfloat();
				bitmap_fs.write(reinterpret_cast<char*>(&img[i]), sizeof(img[i]));
			}
			bitmap_fs.close();
		}
		// Create solution_file
		constexpr const float kernel[3][3] = {
			{ 0.0625f, 0.125f, 0.0625f },
			{ 0.125f, 0.25f, 0.125f },
			{ 0.0625f, 0.125f, 0.0625f }
		};
		std::string sol_path = std::filesystem::temp_directory_path() / ("sol-" + id + ".bmp");
		std::cout << "[3/6] Looking for verification file " << sol_path << std::endl;
		if(std::filesystem::exists(sol_path)) std::cout << "[4/6] Verification file found, using existing verification data" << std::endl;
		else{
			std::cout << "[4/6] Verification file not found. Creating new verification data" << std::endl;
			std::ofstream sol_fs(sol_path, std::ios::binary);
			for(std::int32_t k = 0; k < num_rows * num_cols; k++) {
				float sum = 0.0;
				int i = k / num_cols, j = k % num_cols;
				for(int di = -1; di <= 1; di++)
					for(int dj = -1; dj <= 1; dj++) {
						int ni = i + di, nj = j + dj;
						if(ni >= 0 and ni < num_rows and nj >= 0 and nj < num_cols) 
							sum += kernel[di+1][dj+1] * img[ni * num_cols + nj];
					}
				sol_fs.write(reinterpret_cast<char*>(&sum), sizeof(sum));
			}
			sol_fs.close();
		}
		std::cout << "[5/6] Running student solution" << std::endl;
		// Time your solution's execution time
		auto start = std::chrono::high_resolution_clock::now();
		const std::string student_sol_path = solution::compute(bitmap_path, kernel, num_rows, num_cols);
		auto stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
		std::cout << "[6/6] Verifying student solution" << std::endl;
		// Verify solution
		std::int32_t fd_sol = open(sol_path.c_str(), O_RDONLY);
		std::int32_t fd_student = open(student_sol_path.c_str(), O_RDONLY);
		const std::size_t file_size = num_rows * num_cols;
		const auto sol_data = reinterpret_cast<float*>(mmap(nullptr, file_size * sizeof(float), PROT_READ, MAP_PRIVATE, fd_sol, 0));
		const auto student_data = reinterpret_cast<float*>(mmap(nullptr, file_size * sizeof(float), PROT_READ, MAP_PRIVATE, fd_student, 0));
		constexpr const float error_threshold = 1e-6;
		std::uint32_t remaining = 0;
		#ifdef __AVX512F__
			const __m512 threshold_vec = _mm512_set1_ps(error_threshold);
			for (std::uint32_t i = 0; i + 16 <= file_size; i += 16) {
				__m512 abs_diff_vec = _mm512_abs_ps(_mm512_sub_ps(_mm512_loadu_ps(sol_data + i), _mm512_loadu_ps(student_data + i)));
				__mmask16 mask = _mm512_cmp_ps_mask(abs_diff_vec, threshold_vec, _CMP_GT_OQ);
				if(mask){
					float error = _mm512_mask_reduce_max_ps(mask, abs_diff_vec);
					__terminate_gracefully("Solution has higher error than required: " + std::to_string(error));
				}
			}
			remaining = file_size % 16;
		#else
			const __m256 threshold_vecp = _mm256_set1_ps(error_threshold);
			const __m256 threshold_vecn = _mm256_set1_ps(-error_threshold);
			for (std::uint32_t i = 0; i+8 <= file_size; i += 8) {
				__m256 diff_vec = _mm256_sub_ps(_mm256_loadu_ps(sol_data + i), _mm256_loadu_ps(student_data + i));
				__m256 maskp = _mm256_cmp_ps(diff_vec, threshold_vecp, _CMP_GT_OQ);
				__m256 maskn = _mm256_cmp_ps(diff_vec, threshold_vecn, _CMP_LT_OQ);
				std::int32_t mask = _mm256_movemask_ps(_mm256_or_ps(maskp, maskn));
				if(mask) __terminate_gracefully("Solution has higher error than required.");
			}
			remaining = file_size % 8;
		#endif
		for(std::uint32_t i=file_size-remaining; i < file_size; i++){
			float diff = std::abs(sol_data[i] - student_data[i]);
			if(diff > error_threshold) __terminate_gracefully("Solution has higher error than required: " + std::to_string(diff));
		}
		munmap(sol_data, file_size * sizeof(float)); munmap(student_data, file_size * sizeof(float));
		close(fd_sol); close(fd_student);
		std::filesystem::remove(student_sol_path);
		std::cout << "Execution time: " << duration.count() << " ms" << std::endl;
	
	} catch(const std::exception &e){
		__terminate_gracefully(e.what());
	}
	return EXIT_SUCCESS;
}
