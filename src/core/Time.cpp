#include "Time.h"

Time::Time()
    : deltaTime(0.0f)
    , elapsedTime(0.0f)
    , fps(0)
    , frameCount(0)
{
    lastTime = std::chrono::high_resolution_clock::now();
    currentTime = lastTime;
    fpsTimer = lastTime;
}

void Time::update() {
    currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> delta = currentTime - lastTime;
    deltaTime = delta.count();
    elapsedTime += deltaTime;
    lastTime = currentTime;

    frameCount++;
    std::chrono::duration<float> fpsDelta = currentTime - fpsTimer;
    if (fpsDelta.count() >= 1.0f) {
        fps = frameCount;
        frameCount = 0;
        fpsTimer = currentTime;
    }
}

float Time::getDeltaTime() const {
    return deltaTime;
}

float Time::getElapsed() const {
    return elapsedTime;
}

int Time::getFPS() const {
    return fps;
}

float Time::toSeconds(int64_t microseconds) {
    return static_cast<float>(microseconds) / 1000000.0f;
}

int64_t Time::toMicroseconds(float seconds) {
    return static_cast<int64_t>(seconds * 1000000.0f);
}
