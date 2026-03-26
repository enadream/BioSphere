#pragma once

#include <chrono>

class Timer {
public:
    Timer();

    // Updates the internal state; should be called once per frame
    void Update();

    // Returns time (in seconds) between the last two Update() calls
    float GetDeltaTime() const {return m_DeltaTime;}

    // Returns total time (in seconds) since creation
    float GetElapsedTime() const;

    // Resets the start time now
    void Reset();

private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    TimePoint m_StartTime;
    TimePoint m_LastFrameTime;
    float m_DeltaTime;
};