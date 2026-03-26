#include "core/timer.hpp"
#include <chrono>

Timer::Timer() : m_DeltaTime(0.0f) {
    m_StartTime = Clock::now();
    m_LastFrameTime = m_StartTime;
}

void Timer::Update() {
    TimePoint currentTime = Clock::now();
    std::chrono::duration<float> frameDuration = currentTime - m_LastFrameTime;
    m_DeltaTime = frameDuration.count();
    m_LastFrameTime = currentTime;
}

float Timer::GetElapsedTime() const {
    std::chrono::duration<float> elapsed = Clock::now() - m_StartTime;
    return elapsed.count();
}

void Timer::Reset() {
    m_StartTime = Clock::now();
    m_LastFrameTime = m_StartTime;
    m_DeltaTime = 0.0f;
}