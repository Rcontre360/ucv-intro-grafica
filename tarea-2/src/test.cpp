#include <cstdio>       // For C-style file I/O (fopen_s, fprintf, fclose)
#include <string.h>     // For strerror_s
#include <cerrno>       // For errno
#include "EllipseRender.h"
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iostream>     // Added for std::cerr and std::cout usage

// --- C-style Directory Utilities (Platform Agnostic) ---
// Using C functions to ensure compatibility across different C++ standards.
#include <sys/stat.h>
#include <direct.h>     // For _mkdir on Windows

// Increase the buffer size for strerror_s, as the default is 98
#define ERROR_BUFFER_SIZE 256

// Helper function to recursively create directories like std::filesystem::create_directories.
// This is necessary because C's _mkdir only creates one level.
bool create_directories_c(const std::string& path) {
    if (path.empty()) return true;

    // Use a forward slash as the separator internally (Windows handles this fine)
    std::string current_path;
    for (size_t i = 0; i < path.length(); ++i) {
        current_path += path[i];
        // Check for separators or end of string.
        if (path[i] == '/' || path[i] == '\\' || i == path.length() - 1) {
            // Skip if the path is just a dot, or a separator at the start.
            if (current_path == "." || current_path.empty() || current_path == "/" || current_path == "\\") continue;
            
            // Check if the path exists
            struct _stat info;
            if (_stat(current_path.c_str(), &info) != 0) {
                // Path does not exist, try to create it
                if (_mkdir(current_path.c_str()) != 0) {
                    // Fail only if the error isn't EEXIST (meaning another process created it)
                    if (errno != EEXIST) {
                        return false;
                    }
                }
            } else if (!(info.st_mode & S_IFDIR)) {
                // Path exists but is not a directory
                return false;
            }
        }
    }
    return true;
}

// Helper to check if a directory exists (used for non-recursive creation)
bool directory_exists_c(const std::string& path) {
    struct _stat info;
    return _stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

// Helper to create a single directory (used for non-recursive creation)
bool create_directory_c(const std::string& path) {
    if (_mkdir(path.c_str()) != 0) {
        return (errno == EEXIST) ? true : false;
    }
    return true;
}
// ---------------------------------------------


class EllipseTest : public EllipseRender
{
    protected:
        std::vector<Point> ellipse1;
        std::vector<Point> ellipse2;

        bool is_benchmark;
        const int BENCHMARK_MAX_ELLIPSES = 1000000;
        const int BENCHMARK_STEP = 5000;

    public:
        // Helper function to check if two ellipses (represented by point vectors) are identical.
        bool isSameEllipse(std::vector<Point> a, std::vector<Point> b){
            // If sizes are different, they can't be the same.
            if (a.size() != b.size()) {
                return false;
            }

            // Simple O(N^2) comparison. For production code, better to use sorting or std::unordered_set.
            for (auto pa:a){
                bool found = false;
                for (auto pb:b)
                    if (pa.x == pb.x && pa.y == pb.y){
                        found = true;
                        break;
                    }

                if (!found)
                    return false;
            }

            return true;
        }

        // Overwrites the base class's setPixel to record points instead of drawing to a screen.
        void setPixel(int x, int y, const RGBA& _){
            if (is_benchmark)
                return;

            if (use_optimized)
                ellipse2.push_back({x,y});
            else
                ellipse1.push_back({x,y});
        }

        // Runs a benchmark comparing the 'optimized' and 'vanilla' ellipse algorithms.
        void benchmark(int h, int w) {
            printf("RUNNING BENCHMARK %ix%i\n",h,w);

            height = h;
            width = w;
            is_benchmark = true;

            const std::string dir = "./benchmark";

            std::stringstream filename_ss;
            filename_ss << dir << "/" << h << "x" << w << ".csv";
            const std::string file_path = filename_ss.str();

            // FIX: Replaced fs::exists and fs::create_directory with C-style functions
            if (!directory_exists_c(dir))
                create_directory_c(dir);

            FILE* file = nullptr;
            // Replaced fopen with the secure version fopen_s
            errno_t err = fopen_s(&file, file_path.c_str(), "w");
            
            if (err != 0 || file == nullptr) {
                char err_msg[ERROR_BUFFER_SIZE];
                // Replaced strerror with the secure version strerror_s
                strerror_s(err_msg, ERROR_BUFFER_SIZE, err);
                std::cerr << "Unable to open file: " << file_path << " Error: " << err_msg << std::endl;
                return;
            }
            fprintf(file, "num_ellipses,time,algorithm\n");

            for (int i = BENCHMARK_STEP; i <= BENCHMARK_MAX_ELLIPSES; i += BENCHMARK_STEP) {

                // we generate random ellipses
                std::vector<Ellipse> ellipses_for_test;
                ellipses_for_test.reserve(i);
                for (int j = 0; j < i; j++) {
                    ellipses_for_test.push_back(generateRandomEllipse());
                }

                // === Optimized Algorithm Benchmark ===
                // warm up the CPU
                use_optimized = true;
                for (const auto& e : ellipses_for_test) {
                    drawEllipse(e);
                }

                // run the actual benchmark
                use_optimized = true;
                auto start_optimized = std::chrono::high_resolution_clock::now();
                for (const auto& e : ellipses_for_test) {
                    drawEllipse(e);
                }
                auto end_optimized = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> diff_optimized = end_optimized - start_optimized;
                fprintf(file, "%i,%.10f,%s\n", i, diff_optimized.count(), "optimized");

                // === Vanilla Algorithm Benchmark ===
                // warm up with the vanilla algorithm (the original one)
                use_optimized = false;
                for (const auto& e : ellipses_for_test) {
                    drawEllipse(e);
                }

                // run benchmarks for the original algo
                use_optimized = false;
                auto start_vanilla = std::chrono::high_resolution_clock::now();
                for (const auto& e : ellipses_for_test) {
                    drawEllipse(e);
                }
                auto end_vanilla = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> diff_vanilla = end_vanilla - start_vanilla;
                fprintf(file, "%i,%.10f,%s\n", i, diff_vanilla.count(), "vanilla");


                // log every 100 steps
                if ((i / BENCHMARK_STEP) % 100 == 0)
                    printf("\033[0;34mbenchmarked %i ellipses\033[0m\n", i);
            }

            fclose(file);
            printf("\tBENCHMARK FINISHED. Results saved to: %s\n", file_path.c_str());
        }

        // Compares the output of the two ellipse algorithms and logs results to files.
        void comparisonTest(int h, int w){
            printf("RUNNING COMPARISON TEST FOR \033[0;34m%ix%i\033[0m SCREEN\n", h,w);

            bool success = true;
            height = h;
            width = w;

            std::stringstream dir_ss;
            dir_ss << "./comparison/" << h << "x" << w;
            std::string dir_path = dir_ss.str();
            
            // FIX: Replaced fs::create_directories with C-style recursive function
            create_directories_c(dir_path);

            for (int i=0; i < 10000; i++){ //10k tests
                Ellipse e = generateRandomEllipse();

                // Draw with Vanilla (original) algorithm
                use_optimized = false;
                drawEllipse(e);

                // Draw with Optimized algorithm
                use_optimized = true;
                drawEllipse(e);

                std::stringstream test_dir_ss;
                test_dir_ss << dir_path << "/test_" << i;
                std::string test_dir_path = test_dir_ss.str();
                
                // FIX: Replaced fs::create_directory with C-style function
                create_directory_c(test_dir_path);

                // --- Write Vanilla results ---
                std::string file1_path = test_dir_path + "/draw_ellipse_1.txt";
                FILE* file1 = nullptr;
                // Replaced fopen with the secure version fopen_s
                errno_t err1 = fopen_s(&file1, file1_path.c_str(), "w");
                
                if (err1 != 0 || file1 == nullptr) {
                    char err_msg[ERROR_BUFFER_SIZE];
                    // Replaced strerror with the secure version strerror_s
                    strerror_s(err_msg, ERROR_BUFFER_SIZE, err1);
                    std::cerr << "Unable to open file: " << file1_path << " Error: " << err_msg << std::endl;
                    return; // Or handle error appropriately
                }
                for (const auto& point : ellipse1) {
                    fprintf(file1, "%i %i\n", point.x, point.y);
                }
                fclose(file1);

                // --- Write Optimized results ---
                std::string file2_path = test_dir_path + "/draw_ellipse_2.txt";
                FILE* file2 = nullptr;
                // Replaced fopen with the secure version fopen_s
                errno_t err2 = fopen_s(&file2, file2_path.c_str(), "w");
                
                if (err2 != 0 || file2 == nullptr) {
                    char err_msg[ERROR_BUFFER_SIZE];
                    // Replaced strerror with the secure version strerror_s
                    strerror_s(err_msg, ERROR_BUFFER_SIZE, err2);
                    std::cerr << "Unable to open file: " << file2_path << " Error: " << err_msg << std::endl;
                    return; // Or handle error appropriately
                }
                for (const auto& point : ellipse2) {
                    fprintf(file2, "%i %i\n", point.x, point.y);
                }
                fclose(file2);

                success &= isSameEllipse(ellipse1, ellipse2);

                clear();
            }

            if (success)
                printf("\033[1;32mSUCCESS\033[0m\n");
            else
                printf("\033[0;31mFAILURE\033[0m\n");

        }

        // Clears the recorded point vectors for the next test run.
        void clear(){
            ellipse1.clear();
            ellipse2.clear();
        }
};

int main() {
    EllipseTest* test = new EllipseTest();

    test->comparisonTest(100,100);
    test->comparisonTest(200,200);
    test->comparisonTest(500,500);
    //// 8k screen
    test->comparisonTest(4000,4000);

    std::cout << "\nComparison tests finished. Press Enter to start benchmarks..." << std::endl;
    std::cin.get();

    test->benchmark(500,500);
    test->benchmark(4000,4000);
    test->benchmark(8000,8000);

    return 0;
}
