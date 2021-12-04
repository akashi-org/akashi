#include "./stage.h"

#include "./core/glc.h"
#include "./render_context.h"
#include "./fbo.h"
#include "./camera.h"
#include "./actors/actor.h"

#include "../../item.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>
#include <libakcore/error.h>

namespace akashi {
    namespace graphics {

        struct StageAux {};

        bool Stage::create(const OGLRenderContext& /* ctx */) {
            this->init_gl();
            return true;
        }

        bool Stage::destroy(const OGLRenderContext& ctx) {
            for (auto&& actor : m_actors) {
                if (actor) {
                    actor->destroy(ctx);
                }
            }
            m_actors.clear();
            m_actor_map.clear();

            if (m_aux) {
                delete m_aux;
            }
            m_aux = nullptr;
            return true;
        }

        bool Stage::render(OGLRenderContext& ctx, const RenderParams& params,
                           const core::FrameContext& frame_ctx) {
            // render layers to fbo
            CHECK_AK_ERROR2(this->render_layers(ctx, frame_ctx));

            // render fbo to the provided framebuffer
            this->init_renderer({params.default_fb, params.screen_width, params.screen_height});

            ctx.fbo().render(ctx.camera()->vp_mat());

            return true;
        }

        bool Stage::encode_render(OGLRenderContext& ctx, const core::FrameContext& frame_ctx) {
            // render layers to fbo
            CHECK_AK_ERROR2(this->render_layers(ctx, frame_ctx));
            return true;
        }

        void Stage::init_gl() {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            // If enabled, we can change the point size in shader
            // glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

            glEnable(GL_MULTISAMPLE);
        }

        void Stage::init_renderer(const FBInfo& info) {
            glBindFramebuffer(GL_FRAMEBUFFER, info.fbo);
            glViewport(0.0, 0.0, info.width, info.height);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        bool Stage::render_layers(OGLRenderContext& ctx, const core::FrameContext& frame_ctx) {
            if (!ctx.fbo().initilized()) {
                AKLOG_WARNN("FBO is not yet initialized");
                return false;
            }

            if (frame_ctx.layer_ctxs.size() == 0 || frame_ctx.layer_ctxs.empty()) {
                AKLOG_WARNN("Layer length is 0");
                return false;
            }

            // activate fbo
            this->init_renderer(ctx.fbo().info());

            // ctx.mut_camera()->update(ctx.fbo().info());

            // when new atom comes
            if (m_current_atom_uuid != frame_ctx.layer_ctxs[0].atom_uuid) {
                AKLOG_DEBUG("new atom: old: {}, new: {}", m_current_atom_uuid,
                            frame_ctx.layer_ctxs[0].atom_uuid);

                // clear
                for (auto&& actor : m_actors) {
                    if (actor) {
                        actor->destroy(ctx);
                    }
                }
                m_actors.clear();
                m_actor_map.clear();

                // add
                for (const auto& layer_ctx : frame_ctx.layer_ctxs) {
                    this->add_layer(ctx, layer_ctx);
                }
                m_current_atom_uuid = frame_ctx.layer_ctxs[0].atom_uuid;
            }
            // when exisiting atom comes
            else {
                for (const auto& layer_ctx : frame_ctx.layer_ctxs) {
                    auto it = m_actor_map.find(layer_ctx.uuid);
                    if (it != m_actor_map.end()) {
                        it->second->update_layer(layer_ctx);
                    }
                }
            }

            // render
            // [XXX] do not know why it fails when using for_each
            for (auto iter = m_actors.rbegin(), end = m_actors.rend(); iter != end; ++iter) {
                const auto& actor = *iter;
                if (actor && actor->get_layer_ctx().display) {
                    CHECK_AK_ERROR2(actor->render(ctx, to_rational(frame_ctx.pts)));
                }
            }

            return true;
        }

        bool Stage::add_layer(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) {
            auto actor = create_actor(ctx, layer_ctx);

            if (actor) {
                m_actors.push_back(actor);
                m_actor_map.insert({layer_ctx.uuid, actor});
                return true;
            } else {
                AKLOG_WARN("skip adding the layer: {}", layer_ctx.uuid);
                return false;
            }
        }

    }
}
