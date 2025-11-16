#include "EllipseRender.h"
#include <unordered_set>
#include <fstream>
#include <chrono>

using namespace std;

class EllipseTest : public EllipseRender
{
    protected:
        vector<Point> ellipse1;
        vector<Point> ellipse2;

        bool is_benchmark;
        const int BENCHMARK_MAX_ELLIPSES = 500000;
        const int BENCHMARK_STEP = 1000;
        const int BENCHMARK_HEIGHT = 1000;
        const int BENCHMARK_WIDTH = 1000;

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

        void benchmark(){
            printf("RUNNING BENCHMARK\n");
            height = BENCHMARK_HEIGHT;
            width = BENCHMARK_WIDTH;
            is_benchmark = true;

            ofstream file("benchmark.csv");
            file << "num_ellipses,time,algorithm\n";
            // single ellipse for all tests. 
            // circle to use both parts of the algorithm the same.
            // on the middle of all and with max space
            Ellipse e = {
                {width / 2, height / 2},
                width / 2 - 1, 
                height / 2 - 1,
                {255,255,255,255}
            };

            for (int i=BENCHMARK_STEP; i <= BENCHMARK_MAX_ELLIPSES; i+=BENCHMARK_STEP){
                use_optimized = false;

                auto start = chrono::high_resolution_clock::now();
                for (int j=1; j <= i; j++){
                    drawEllipse(e);
                }
                auto end = chrono::high_resolution_clock::now();
                chrono::duration<double> diff = end-start;

                file << i << "," << diff.count() << "," << "vanilla\n";

                use_optimized = true;

                start = chrono::high_resolution_clock::now();
                for (int j=1; j <= i; j++){
                    drawEllipse(e);
                }
                end = chrono::high_resolution_clock::now();
                diff = end-start;

                file << i << "," << diff.count() << "," << "optimized\n";

                // log every 100 steps
                if ((i/BENCHMARK_STEP)%100 == 0)
                    printf("\033[0;34mbenchmarked %i ellipses\033[0m\n",i);
            }

            printf("\tBENCHMARK FINISHED\n");
        }

        void comparisonTest(int h, int w){
            printf("RUNNING COMPARISON TEST FOR \033[0;34m%ix%i\033[0m SCREEN\n", h,w);

            bool success = true;
            height = h;
            width = w;

            for (int i=0; i < 1000; i++){
                Ellipse e = generateRandomEllipse();

                use_optimized = false;
                drawEllipse(e);

                use_optimized = true;
                drawEllipse(e);

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
    test->benchmark();

    return 0;
}
