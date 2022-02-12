#pragma once

#include "./widget.h"

#include <string>

namespace akashi {
    namespace graphics {

        class OSCRenderContext;

        namespace osc {

            class Timecode final : public BaseWidget {
              public:
                struct Context;

              public:
                explicit Timecode(OSCRenderContext& render_ctx, const BoundingBox& bbox);
                virtual ~Timecode();

                virtual bool update(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool render(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool on_time_event(OSCRenderContext& render_ctx,
                                           const OSCTimeEvent& event) override;

              private:
                std::string construct_time_string();

                std::string formatted_frame_count(long count, int pad_digit = 8);

              private:
                Timecode::Context* m_ctx = nullptr;
            };
        }

    }
}
