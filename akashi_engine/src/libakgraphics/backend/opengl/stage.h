#pragma once

#include "./fbo.h"

#include <libakcore/element.h>
#include <libakcore/memory.h>

#include <unordered_map>
#include <string>
#include <vector>

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace graphics {

        class OGLRenderContext;
        struct RenderParams;
        class LayerObject;
        class Camera;

        class Stage;
        class RenderPlane final {
          public:
            explicit RenderPlane(OGLRenderContext& render_ctx, const core::PlaneContext& plane_ctx,
                                 const core::AtomStaticProfile& atom_static_profile);

            virtual ~RenderPlane() = default;

            void destroy(const OGLRenderContext& ctx);

            bool render(OGLRenderContext& ctx, const core::Rational& pts, const Stage& stage);

            const core::LayerContext& base_layer() const { return m_base_layer; }

            const core::PlaneContext& plane_ctx() const { return m_plane_ctx; }

            void update(const core::PlaneContext& plane_ctx);

            bool is_defunct() const { return m_is_defunct; }

            void set_defunct(bool defunct) { m_is_defunct = defunct; }

          private:
            bool add_layer(OGLRenderContext& ctx, const core::LayerContext& layer_ctx,
                           const core::Rational& pts);

          private:
            FBO m_fbo;
            core::owned_ptr<Camera> m_camera;
            core::PlaneContext m_plane_ctx;
            core::LayerContext m_base_layer;
            core::AtomStaticProfile m_atom_static_profile;

            bool m_initial_render = true;
            bool m_is_defunct = false;

            std::vector<LayerObject*> m_layer_objects;
            std::unordered_map<std::string, LayerObject*> m_layer_object_map;
        };

        struct StageAux;

        class Stage final {
            using uuid_t = std::string;
            using atom_uuid_t = std::string;

          public:
            explicit Stage() = default;
            virtual ~Stage() = default;

            bool create(const OGLRenderContext& ctx);
            bool render(OGLRenderContext& ctx, const RenderParams& params,
                        const core::FrameContext& frame_ctx);

            bool encode_render(OGLRenderContext& ctx, const core::FrameContext& frame_ctx);

            bool destroy(const OGLRenderContext& ctx);

            bool find_render_plane(RenderPlane** render_plane, const std::string& unit_uuid) const;

          private:
            void init_gl();

            bool render_planes(OGLRenderContext& ctx, const core::FrameContext& frame_ctx);

            bool add_plane(OGLRenderContext& ctx, const core::PlaneContext& plane_ctx,
                           const core::AtomStaticProfile& atom_static_profile);

            void destroy_planes(const OGLRenderContext& ctx);

          private:
            std::vector<RenderPlane*> m_planes;
            std::unordered_map<uuid_t, RenderPlane*> m_plane_map;
            atom_uuid_t m_current_atom_uuid;
            StageAux* m_aux = nullptr;
        };
    }
}
