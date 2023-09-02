#pragma once

#include <libakcore/element.h>
#include "../../widgets/widget.h"
#include <cstddef>

typedef struct SDL_Surface SDL_Surface;

namespace akashi {
    namespace graphics {

        class OGLTexture;

        class OSCRenderContext;
        struct RenderParams;

        namespace osc {

            class SeekRuler final : public BaseWidget {
              public:
                struct Context;
                struct LabelParams : core::TextTField {
                    std::array<int32_t, 4> pad = {0, 0, 0, 0};
                    int32_t line_span = 0;
                    core::TextStyleTField style;
                };

                struct Params {
                    size_t seek_ruler = 10;
                    long seek_area_width = 0;
                    LabelParams label;
                    std::array<size_t, 4> label_texts = {0, 0, 0, 0};
                };

              public:
                explicit SeekRuler(OSCRenderContext& render_ctx, const BoundingBox& bbox,
                                   const SeekRuler::Params& init_params);
                virtual ~SeekRuler();

                void update_obj_params(const SeekRuler::Params& obj_params);

                const SeekRuler::Params& obj_params() const { return m_obj_params; }

                void set_label_dirty(bool label_dirty);

                virtual bool update(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool render(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

              private:
                bool load_line_pass();

                bool render_line_pass(OSCRenderContext& render_ctx, const RenderParams& params);

                bool load_label_pass(OSCRenderContext& render_ctx);

                bool load_texture(OSCRenderContext& render_ctx);

                std::string format_label(OSCRenderContext& render_ctx, long frame_num);

                bool create_font_surface(SDL_Surface*& surface, const std::string& text,
                                         const SeekRuler::LabelParams& label_params) const;

                bool load_label_mesh();

                bool render_label_pass(OSCRenderContext& render_ctx, const RenderParams& params);

              private:
                SeekRuler::Context* m_ctx = nullptr;
                SeekRuler::Params m_obj_params;
                bool m_label_dirty = false;
            };

        }

    }
}
