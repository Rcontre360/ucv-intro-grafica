#pragma once

#include <numeric>
#include <vector>
#include <stdexcept>

using namespace std;

class FPSAvgCounter {
private:
    double lastFrameUpdate = 0.0; 
    double lastDeltaTimeUpdate = 0.0;
    int frameCount = 0; 
    int lastXsecondsAvg = 0; 

    // frames / second on the last x seconds
    vector<double> lastCumulativeFPS;
public:
    FPSAvgCounter(int lastXseconds) {
        if (lastXseconds <= 0)
            throw invalid_argument("FPSAvgCounter arg must be at least 1");

        lastXsecondsAvg = lastXseconds;
    }

    double getCount(){
        if (lastCumulativeFPS.empty()) return 0.0;
        return accumulate(lastCumulativeFPS.begin(), lastCumulativeFPS.end(), 0.0) / lastCumulativeFPS.size();
    }

    double getDelta(double currentFrameTime){
        double delta = currentFrameTime - lastDeltaTimeUpdate;
        lastDeltaTimeUpdate = currentFrameTime;
        return delta; 
    }

    void framesPerSecondAvg(double currentFrameTime){
        double deltaTime = currentFrameTime - lastFrameUpdate; 

        frameCount++;
        if (deltaTime >= 1.0) { 
            lastCumulativeFPS.push_back((float)frameCount / (float)deltaTime);

            frameCount = 0;
            lastFrameUpdate = currentFrameTime; 

            if (lastCumulativeFPS.size() > lastXsecondsAvg){
                lastCumulativeFPS.erase(lastCumulativeFPS.begin());
            }
        }
    }
};
