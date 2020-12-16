#pragma once

#include <libakcore/rational.h>
#include <chrono>

namespace akashi {
    namespace player {

        class PerfMonitor final {
          public:
            explicit PerfMonitor() = default;
            virtual ~PerfMonitor() = default;

            void log_delay(const core::Rational& delay, size_t drop_cnt);

            void log_fps(const core::Rational& current_time, const core::Rational& audio_time);

            core::Rational elapsed_time(void) const { return m_elapsed_time; }

          private:
            core::Rational m_delay_sum = core::Rational(0l);
            core::Rational m_elapsed_time = core::Rational(0l);
            size_t m_loop_cnt = 0;
            size_t m_drop_cnt = 0;
        };

        class Timer final {
          private:
            using ClockTime = decltype(std::chrono::high_resolution_clock::now());

          public:
            explicit Timer() = default;
            ~Timer() = default;

            void start() {
                m_started = true;
                m_start_time = this->now();
            }

            void stop() { m_elapsed_time = this->current_time(); }

            core::Rational current_time(void) const;

          private:
            ClockTime now(void) const { return std::chrono::high_resolution_clock::now(); }

          private:
            bool m_started = false;
            ClockTime m_start_time;
            core::Rational m_elapsed_time;
        };

    }
}
