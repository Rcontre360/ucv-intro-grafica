#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include "src/median.h" // Assuming median.h is in src/

// Function to generate a random vector of unsigned char
std::vector<unsigned char> generate_random_vector(int size) {
    std::vector<unsigned char> vec(size);
    // Use a random device to seed the generator
    std::random_device rd;
    // Use the Mersenne Twister engine
    std::mt19937 gen(rd());
    // Define the distribution for unsigned char (0-255)
    std::uniform_int_distribution<> distrib(0, 255);

    for (int i = 0; i < size; ++i) {
        vec[i] = static_cast<unsigned char>(distrib(gen));
    }
    return vec;
}

int main() {
    std::cout << "Running quick_select iterative vs recursive comparison tests..." << std::endl;

    const int num_tests = 100;
    const int max_vec_size = 100;

    for (int test_idx = 0; test_idx < num_tests; ++test_idx) {
        // Generate a random vector size
        int vec_size = (test_idx % max_vec_size) + 1; // Ensure size is at least 1
        std::vector<unsigned char> original_vec = generate_random_vector(vec_size);

        for (int k = 1; k <= vec_size; ++k) {
            std::vector<unsigned char> vec_recursive = original_vec;
            std::vector<unsigned char> vec_iterative = original_vec;

            int result_recursive = quick_select(vec_recursive, 0, vec_size - 1, k);
            int result_iterative = quick_select_iter(vec_iterative, 0, vec_size - 1, k);

            if (result_recursive != result_iterative) {
                std::cerr << "Test failed for vector size " << vec_size << ", k = " << k << std::endl;
                std::cerr << "Original vector: ";
                for (unsigned char val : original_vec) {
                    std::cerr << (int)val << " ";
                }
                std::cerr << std::endl;
                std::cerr << "Recursive result: " << result_recursive << std::endl;
                std::cerr << "Iterative result: " << result_iterative << std::endl;
                return 1; // Indicate failure
            }
        }
    }

    std::cout << "All quick_select iterative vs recursive comparison tests passed!" << std::endl;

    return 0; // Indicate success
}
