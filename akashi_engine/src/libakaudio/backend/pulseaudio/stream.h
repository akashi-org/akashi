#pragma once

#include <libakcore/memory.h>

typedef struct pa_stream pa_stream;
typedef struct pa_threaded_mainloop pa_threaded_mainloop;
typedef struct pa_context pa_context;

namespace akashi {
    namespace audio {

        class PulseAudioContext;
        class AudioStream final {
          private:
            constexpr static const char STREAM_NAME[] = "akashi-pa-stream";

          public:
            explicit AudioStream(core::borrowed_ptr<PulseAudioContext> audio_ctx,
                                 pa_threaded_mainloop* mainloop, pa_context* context);

            // [XXX] all the resources in this class should be managed
            // by its owner PulseAudioContext
            virtual ~AudioStream() = default;

            void init(void);

            void cork(void);

            void uncork(void);

            void flush(void);

            void drain(void);

            void destroy(void);

            pa_stream* stream(void) const { return m_stream; }

          private:
            static void stream_state_cb(pa_stream* s, void* mainloop);

            static void stream_success_cb(pa_stream* stream, int success, void* userdata);

          private:
            core::borrowed_ptr<PulseAudioContext> m_audio_ctx;
            pa_threaded_mainloop* m_mainloop = nullptr;
            pa_context* m_context = nullptr;
            pa_stream* m_stream = nullptr;
        };

    }
}
