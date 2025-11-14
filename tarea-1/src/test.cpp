#include "EllipseRender.h"
#include <unordered_set>

using namespace std;

class EllipseTest : public EllipseRender
{
    protected:
        vector<Point> ellipse1;
        vector<Point> ellipse2;

    public:
        bool isSameEllipse(vector<Point> a, vector<Point> b){
            // at least 1 point of difference is wrong
            int cnt = 0;
            for (auto pa:a){
                bool found = false;
                for (auto pb:b)
                    if (pa.x == pb.x && pa.y == pb.y){
                        found = true;
                        break;
                    }

                if (!found)
                    cnt++;
            }

            printf("diff on %i points\n", cnt);
            // must have same number of points
            return a.size() == b.size() && cnt == 0;
        }

        //overwritting set pixel
        void setPixel(int x, int y, const RGBA& _){
            if (use_optimized)
                ellipse2.push_back({x,y});
            else
                ellipse1.push_back({x,y});
        }

        void comparisonTest(){
            height = 15000;
            width = 15000;
            Ellipse e = generateRandomEllipse();

            use_optimized = false;
            drawEllipse(e);

            use_optimized = true;
            drawEllipse(e);

            printf("created %i and %i points\n", ellipse1.size(),ellipse1.size());
            if (isSameEllipse(ellipse1, ellipse2))
                printf("SUCCESS\n");
            else
                printf("FAILURE\n");
        }
};

int main() {
    EllipseTest* test = new EllipseTest();

    test->comparisonTest();
    //delete test;

    return 0;
}
