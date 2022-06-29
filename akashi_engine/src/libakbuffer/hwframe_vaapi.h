#pragma once

#include "./hwframe.h"

#include <libakcore/memory.h>

#include <va/va.h>

namespace akashi {
    namespace buffer {

        class NativeFrame;

        struct HWFrameInfo {
            VADisplay va_display;
            VASurfaceID va_surface_id;
        };

        class VAAPIHWFrame final : public HWFrame {
            AK_FORBID_COPY(VAAPIHWFrame)
          public:
            enum class NativeFrameKind { FFMPEG };

          public:
            explicit VAAPIHWFrame(NativeFrame* frame, VADisplay va_display);

            virtual ~VAAPIHWFrame();

            virtual HWFrameInfo hw_info() const override;

            void set_gfx_hwctx(core::owned_ptr<GFXHWContext> gfx_hwctx) override;

            NativeFrame* native_frame() const;

            NativeFrameKind native_frame_kind() const;

          private:
            NativeFrame* m_frame = nullptr;
            HWFrameInfo m_hw_info;
            core::owned_ptr<GFXHWContext> m_gfx_hwctx = nullptr;
        };

    }
}
