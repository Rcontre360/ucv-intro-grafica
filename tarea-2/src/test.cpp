#include <cstdio> // For C-style file I/O (fopen_s, fprintf, fclose)
#include <string.h> // For strerror_s
#include "EllipseRender.h"
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>

class EllipseTest : public EllipseRender
{
    protected:
        std::vector<Point> ellipse1;
        std::vector<Point> ellipse2;

        bool is_benchmark;
        const int BENCHMARK_MAX_ELLIPSES = 10000;
        const int BENCHMARK_STEP = 5000;

    public:
        bool isSameEllipse(std::vector<Point> a, std::vector<Point> b){
            // at least 1 point of difference is wrong
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

            return a.size() == b.size();
        }

        //overwritting set pixel
        void setPixel(int x, int y, const RGBA& _){
            if (is_benchmark)
                return;

            if (use_optimized)
                ellipse2.push_back({x,y});
            else
                ellipse1.push_back({x,y});
        }

        // benchmark, was hard to setup because of caching issues and having to use random ellipses
        void benchmark(int h, int w) {
            printf("RUNNING BENCHMARK %ix%i\n",h,w);

            height = h;
            width = w;
            is_benchmark = true;

            const std::string dir = "./benchmark";

            std::stringstream filename_ss;
            filename_ss << dir << "/" << h << "x" << w << ".csv";
            const std::string file_path = filename_ss.str();

            if (!std::filesystem::exists(dir))
                std::filesystem::create_directory(dir);

            FILE* file = fopen(file_path.c_str(), "w");
            if (file == nullptr) {
                std::cerr << "Unable to open file: " << file_path << " Error: " << strerror(errno) << std::endl;
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

                // we warm up the CPU (fill its cache). So we run the optimized algo with all ellipses
                use_optimized = true;
                for (const auto& e : ellipses_for_test) {
                    drawEllipse(e);
                }

                // we run the actual benchmark
                use_optimized = true;
                auto start_optimized = std::chrono::high_resolution_clock::now();
                for (const auto& e : ellipses_for_test) {
                    drawEllipse(e);
                }
                auto end_optimized = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> diff_optimized = end_optimized - start_optimized;
                fprintf(file, "%i,%.10f,%s\n", i, diff_optimized.count(), "optimized");

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

        // not only compares but creates a "comparison" directory with all the tests results
        void comparisonTest(int h, int w){
            printf("RUNNING COMPARISON TEST FOR \033[0;34m%ix%i\033[0m SCREEN\n", h,w);

            bool success = true;
            height = h;
            width = w;

            std::stringstream dir_ss;
            dir_ss << "./comparison/" << h << "x" << w;
            std::string dir_path = dir_ss.str();
            std::filesystem::create_directories(dir_path);

            for (int i=0; i < 10000; i++){ //10k tests
                Ellipse e = generateRandomEllipse();

                use_optimized = false;
                drawEllipse(e);

                use_optimized = true;
                drawEllipse(e);

                std::stringstream test_dir_ss;
                test_dir_ss << dir_path << "/test_" << i;
                std::string test_dir_path = test_dir_ss.str();
                std::filesystem::create_directory(test_dir_path);

                std::string file1_path = test_dir_path + "/draw_ellipse_1.txt";
                FILE* file1 = fopen(file1_path.c_str(), "w");
                if (file1 == nullptr) {
                    std::cerr << "Unable to open file: " << file1_path << " Error: " << strerror(errno) << std::endl;
                    return; // Or handle error appropriately
                }
                for (const auto& point : ellipse1) {
                    fprintf(file1, "%i %i\n", point.x, point.y);
                }
                fclose(file1);

                std::string file2_path = test_dir_path + "/draw_ellipse_2.txt";
                FILE* file2 = fopen(file2_path.c_str(), "w");
                if (file2 == nullptr) {
                    std::cerr << "Unable to open file: " << file2_path << " Error: " << strerror(errno) << std::endl;
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
    //test->comparisonTest(4000,4000);

    std::cout << "\nComparison tests finished. Press Enter to start benchmarks..." << std::endl;
    std::cin.get();

    test->benchmark(500,500);
    //test->benchmark(4000,4000);
    //test->benchmark(8000,8000);

    return 0;
}
