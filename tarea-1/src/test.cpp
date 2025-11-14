#include "EllipseRender.h"

using namespace std;

typedef uniform_int_distribution<int> Dist;

class EllipseTest : public EllipseRender
{
    protected:
        vector<Point> ellipse1;
        vector<Point> ellipse2;

    public:
        bool isSameEllipse(vector<Point> a, vector<Point> b){
            // at least 1 point of difference is wrong
            for (int i = 0; i < a.size(); i++)
                if (a[i].x != b[i].x || a[i].y != b[i].y)
                    return false;

            // must have same number of points
            return a.size() == b.size();
        }

        //overwritting set pixel
        void setPixel(int x, int y, const RGBA& _){
            if (use_optimized)
                ellipse2.push_back({x,y});
            else
                ellipse1.push_back({x,y});
        }

        void comparisonTest(){
            height = 10000;
            width = 10000;
            Ellipse e = generateRandomEllipse();

            use_optimized = false;
            drawEllipse(e);

            use_optimized = true;
            drawEllipse(e);
        }
};

int main() {
    EllipseTest* test = new EllipseTest();

    test->comparisonTest();
    //delete test;

    return 0;
}
