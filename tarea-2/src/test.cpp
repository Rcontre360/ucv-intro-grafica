#include "EllipseRender.h"
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

class EllipseTest : public EllipseRender
{
    protected:
        vector<Point> ellipse1;
        vector<Point> ellipse2;

        bool is_benchmark;
        const int BENCHMARK_MAX_ELLIPSES = 1000000;
        const int BENCHMARK_STEP = 10000;

    public:
        bool isSameEllipse(vector<Point> a, vector<Point> b){
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

        void benchmark(int h, int w) {
            printf("RUNNING BENCHMARK %ix%i\n",h,w);
            
            height = h;
            width = w;
            is_benchmark = true;

            const string dir = "./benchmark";

            stringstream filename_ss;
            filename_ss << dir << "/" << h << "x" << w << ".csv";
            const string file_path = filename_ss.str();

            if (!fs::exists(dir)) 
                fs::create_directory(dir);

            ofstream file(file_path);
            file << "num_ellipses,time,algorithm\n";

            for (int i = BENCHMARK_STEP; i <= BENCHMARK_MAX_ELLIPSES; i += BENCHMARK_STEP) {
                
                vector<Ellipse> ellipses_for_test;
                ellipses_for_test.reserve(i);
                for (int j = 0; j < i; j++) {
                    ellipses_for_test.push_back(generateRandomEllipse());
                }

                //VANILLA ALGO
                use_optimized = false;
                auto start_vanilla = chrono::high_resolution_clock::now();
                for (const auto& e : ellipses_for_test) {
                    drawEllipse(e);
                }
                auto end_vanilla = chrono::high_resolution_clock::now();
                chrono::duration<double> diff_vanilla = end_vanilla - start_vanilla;

                file << i << "," << diff_vanilla.count() << "," << "vanilla\n";

                // OPTIMIZED ALGO
                use_optimized = true;
                auto start_optimized = chrono::high_resolution_clock::now();
                for (const auto& e : ellipses_for_test) {
                    drawEllipse(e);
                }
                auto end_optimized = chrono::high_resolution_clock::now();
                chrono::duration<double> diff_optimized = end_optimized - start_optimized;

                file << i << "," << diff_optimized.count() << "," << "optimized\n";

                // log every 100 steps
                if ((i / BENCHMARK_STEP) % 100 == 0)
                    printf("\033[0;34mbenchmarked %i ellipses\033[0m\n", i);
            }

            file.close();
            printf("\tBENCHMARK FINISHED. Results saved to: %s\n", file_path.c_str());
        }

        void comparisonTest(int h, int w){
            printf("RUNNING COMPARISON TEST FOR \033[0;34m%ix%i\033[0m SCREEN\n", h,w);

            bool success = true;
            height = h;
            width = w;

            stringstream dir_ss;
            dir_ss << "./comparison/" << h << "x" << w;
            string dir_path = dir_ss.str();
            fs::create_directories(dir_path);

            for (int i=0; i < 50; i++){
                Ellipse e = generateRandomEllipse();

                use_optimized = false;
                drawEllipse(e);

                use_optimized = true;
                drawEllipse(e);

                stringstream test_dir_ss;
                test_dir_ss << dir_path << "/test_" << i;
                string test_dir_path = test_dir_ss.str();
                fs::create_directory(test_dir_path);

                ofstream file1(test_dir_path + "/draw_ellipse_1.txt");
                for (const auto& point : ellipse1) {
                    file1 << point.x << " " << point.y << "\n";
                }
                file1.close();

                ofstream file2(test_dir_path + "/draw_ellipse_2.txt");
                for (const auto& point : ellipse2) {
                    file2 << point.x << " " << point.y << "\n";
                }
                file2.close();

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
    // 8k screen
    test->comparisonTest(8000,8000);

    cout << "\nComparison tests finished. Press Enter to start benchmarks..." << endl;
    cin.get();

    test->benchmark(4000,4000);

    return 0;
}
