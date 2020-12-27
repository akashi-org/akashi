#pragma once

#include "./logger.h"
#include "./rational.h"
#include <chrono>

namespace akashi {
    namespace core {

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

            void reset() { m_elapsed_time = core::Rational(0l); }

            core::Rational current_time(void) const {
                if (!m_started) {
                    AKLOG_ERRORN("not started yet");
                    return Rational(-1, 1);
                }
                return Rational(std::chrono::duration_cast<std::chrono::milliseconds>(this->now() -
                                                                                      m_start_time)
                                    .count(),
                                1000) +
                       m_elapsed_time;
            }

          private:
            ClockTime now(void) const { return std::chrono::high_resolution_clock::now(); }

          private:
            bool m_started = false;
            ClockTime m_start_time;
            core::Rational m_elapsed_time;
        };

        class PerfMonitor final {
          public:
            explicit PerfMonitor() = default;
            virtual ~PerfMonitor() = default;

            void log_delay(const core::Rational& delay, size_t drop_cnt) {
                m_delay_sum += delay;
                m_loop_cnt += 1;
                m_drop_cnt += drop_cnt;
                auto av = m_delay_sum / Rational(m_loop_cnt, 1);
                AKLOG_INFO("delay: {}, av_delay: {}, drop_cnt: {} / {}", delay.to_decimal(),
                           av.to_decimal(), m_drop_cnt, m_loop_cnt);
            }

            void log_fps(const core::Rational& current_time, const core::Rational& audio_time) {
                auto elapsed = audio_time - m_elapsed_time;
                auto fps = elapsed > 0 ? Rational(1l) / elapsed : Rational(-1l);
                AKLOG_DEBUG("current_time: {:05.4f} sec, fps: {}", current_time.to_decimal(),
                            fps.to_decimal());
                // [TODO] a process is needed that elapsed_time is set back to zero after a loop
                m_elapsed_time = audio_time;
            }

            void log_render_start() {
                m_render_timer.reset();
                m_render_timer.start();
            }

            void log_render_end(const char* header) {
                m_render_timer.stop();
                m_render_cnt += 1;
                if (m_render_cnt == 1) {
                    m_av_render_time = m_render_timer.current_time();
                } else {
                    // a[n] = (((n-1) * a[n-1]) + x[n]) / n
                    m_av_render_time = (core::Rational(m_render_cnt - 1, 1) * m_av_render_time +
                                        m_render_timer.current_time()) /
                                       core::Rational(m_render_cnt, 1);
                }
                AKLOG_DEBUG("{}: {:05.4f} sec (av: {}, count: {})", header,
                            m_render_timer.current_time().to_decimal(),
                            m_av_render_time.to_decimal(), m_render_cnt);
            }

            core::Rational elapsed_time(void) const { return m_elapsed_time; }

            core::Rational av_render_time(void) const { return m_av_render_time; }

          private:
            core::Rational m_delay_sum = core::Rational(0l);
            core::Rational m_elapsed_time = core::Rational(0l);
            size_t m_loop_cnt = 0;
            size_t m_drop_cnt = 0;
            size_t m_render_cnt = 0;
            Timer m_render_timer;
            core::Rational m_av_render_time = core::Rational(0l);
        };

    }
}
