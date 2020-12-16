#pragma once

typedef struct pa_threaded_mainloop pa_threaded_mainloop;
typedef struct pa_context pa_context;

namespace akashi {
    namespace core {
        enum class AKAudioSampleFormat;
    }
    namespace audio {

        constexpr static unsigned int MAX_AUDIO_BUFFER_SIZE = 1024 * 10; // 10kb

        // [TODO] playback status should be judged from the playable times instead
        const static unsigned int MIN_PLAYABLE_QUEUE_SIZE = 1024 * 10; // 10kb

        template <class T>
        struct PAOpContext {
            T data;
            bool executed = false;
            pa_threaded_mainloop* mainloop = nullptr;
        };

        class MainloopLockGuard final {
          public:
            explicit MainloopLockGuard(pa_threaded_mainloop* mainloop);
            virtual ~MainloopLockGuard();

          private:
            pa_threaded_mainloop* m_mainloop;
        };

        void context_inspect(pa_context* context);

        struct PASampleFormat;
        PASampleFormat to_pl_sample_format(const core::AKAudioSampleFormat& format);

    }
}
