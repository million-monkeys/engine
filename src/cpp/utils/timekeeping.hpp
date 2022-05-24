#pragma once

#include <chrono>
#include <spdlog/spdlog.h>

namespace timekeeping {
    using Clock = std::chrono::high_resolution_clock;
    using DeltaTimeDuration = std::chrono::duration<float>;
    using ElapsedTime = long long;
    using ElapsedTimeDuration = std::chrono::duration<ElapsedTime>;

    constexpr ElapsedTime seconds (ElapsedTime s) { return s * 1000000L; }
    constexpr ElapsedTime millis (ElapsedTime ms) { return ms * 1000L; }

    inline float as_seconds (ElapsedTime micros) { return micros / 1000000.0; }

    class FrameTimer {
    public:
        FrameTimer () :
            m_time_since_start(0),
            m_frame_time_micros(0),
            m_start_time(Clock::now()),
            m_previous_time(m_start_time),
            m_current_time(m_start_time),
            m_total_frames(0)
        {}
        ~FrameTimer () {}

        float sinceStart () const { return as_seconds(m_time_since_start); }
        float frameTime () const { return as_seconds(m_frame_time_micros); }
        uint64_t totalFrames () const { return m_total_frames; }

        void update ()
        {
            m_previous_time = m_current_time;
            m_current_time = Clock::now();
            m_frame_time_micros = std::chrono::duration_cast<std::chrono::microseconds>(m_current_time - m_previous_time).count();
            SPDLOG_TRACE("Frame {} took: {} μs", m_total_frames, m_frame_time_micros);
            if (m_frame_time_micros < seconds(1)) /* Needs clang 12: [[likely]] */ {
                // Accumulate time. Do this rather than measuring from the start time so that we can 'omit' time, eg when paused.
                m_time_since_start += m_frame_time_micros;
            } else {
                // If frame took over a second, assume debugger breakpoint
                m_previous_time = m_current_time;
            }
            ++m_total_frames;
        }

        void reportAverage () const
        {
            auto micros_per_frame = m_time_since_start / m_total_frames;
            if (micros_per_frame == 0) {
                micros_per_frame = 1;
            }
            spdlog::info("Average framerate: {:d} ({:.3f}ms per frame)", 1000000L / micros_per_frame, micros_per_frame / 1000.0f);
        }


    private:
        ElapsedTime m_time_since_start;
        ElapsedTime m_frame_time_micros;
        Clock::time_point m_start_time;
        Clock::time_point m_previous_time;
        Clock::time_point m_current_time;
        uint64_t m_total_frames;
    };
}
