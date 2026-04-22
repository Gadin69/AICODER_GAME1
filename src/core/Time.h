#pragma once

#include <chrono>

class Time {
public:
    Time();

    void update();
    float getDeltaTime() const;
    float getElapsed() const;
    int getFPS() const;

    static float toSeconds(int64_t microseconds);
    static int64_t toMicroseconds(float seconds);

private:
    std::chrono::high_resolution_clock::time_point lastTime;
    std::chrono::high_resolution_clock::time_point currentTime;
    float deltaTime;
    float elapsedTime;
    int fps;
    int frameCount;
    std::chrono::high_resolution_clock::time_point fpsTimer;
};
