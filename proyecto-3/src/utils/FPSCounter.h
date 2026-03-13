#pragma once

class FPSCounter {
    double lastTime = 0.0;
    int frameCount = 0;
    float fps = 0.0f;
public:
    void tick(double currentTime) {
        frameCount++;
        if (currentTime - lastTime >= 1.0) {
            fps = frameCount / (float)(currentTime - lastTime);
            frameCount = 0;
            lastTime = currentTime;
        }
    }
    float get() const { return fps; }
};
