#include "./render.h"

#include "../../item.h"
#include "./gl.h"
#include "./framebuffer.h"
#include "./objects/quad/quad.h"
#include "./layer.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>
#include <libakcore/error.h>
#include <libakcore/rational.h>

#include <memory>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        bool RenderScene::create(const GLRenderContext&) { return true; }

        bool RenderScene::render(core::borrowed_ptr<GLGraphicsContext> glx_ctx,
                                 const GLRenderContext& ctx, const RenderParams& params,
                                 const core::FrameContext& frame_ctx) {
            if (!ctx.fbo) {
                AKLOG_WARNN("RenderScene:render(): FBO is not yet initialized");
                return false;
            }

            if (frame_ctx.layer_ctxs.size() == 0 || frame_ctx.layer_ctxs.empty()) {
                AKLOG_WARNN("RenderScene:render(): Layer length is 0");
                return false;
            }

            // be careful about the lifetime of fbo_prop
            const auto& fbo_prop = ctx.fbo->get_prop();
            // activate fbo
            this->render_init(ctx, {fbo_prop.fbo, fbo_prop.width, fbo_prop.height});

            // when new atom is came
            if (m_current_atom_uuid != frame_ctx.layer_ctxs[0].atom_uuid) {
                AKLOG_DEBUG("new atom: old: {}, new: {}", m_current_atom_uuid,
                            frame_ctx.layer_ctxs[0].atom_uuid);

                // clear
                for (auto&& target : m_targets) {
                    if (target) {
                        target->destroy(ctx);
                    }
                }
                m_targets.clear();
                m_target_map.clear();

                // add
                for (const auto& layer_ctx : frame_ctx.layer_ctxs) {
                    this->add_layer(ctx, layer_ctx);
                }
                m_current_atom_uuid = frame_ctx.layer_ctxs[0].atom_uuid;
            }
            // when exisiting atom is came
            else {
                for (const auto& layer_ctx : frame_ctx.layer_ctxs) {
                    auto it = m_target_map.find(layer_ctx.uuid);
                    if (it != m_target_map.end()) {
                        it->second->update_layer(layer_ctx);
                    }
                }
            }

            // render
            // [XXX] do not know why it fails when using for_each
            for (auto iter = m_targets.rbegin(), end = m_targets.rend(); iter != end; ++iter) {
                const auto& target = *iter;
                if (target && target->get_layer_ctx().display) {
                    CHECK_AK_ERROR2(target->render(glx_ctx, ctx, to_rational(frame_ctx.pts)));
                }
            }

            // [TODO] really necessary to call twice?
            GET_GLFUNC(ctx, glFinish)();

            // deactivate fbo
            this->render_init(ctx, {params.default_fb, params.screen_width, params.screen_height});
            ctx.fbo->render(ctx);

            GET_GLFUNC(ctx, glFinish)();

            return true;
        }

        bool RenderScene::destroy(const GLRenderContext& ctx) {
            for (auto&& target : m_targets) {
                if (target) {
                    target->destroy(ctx);
                }
            }
            m_targets.clear();
            m_target_map.clear();
            return true;
        }

        bool RenderScene::render_init(const GLRenderContext& ctx,
                                      const RenderInitParams& params) const {
            GET_GLFUNC(ctx, glBindFramebuffer)(GL_FRAMEBUFFER, params.fb);
            GET_GLFUNC(ctx, glViewport)(0.0, 0.0, params.width, params.height);
            GET_GLFUNC(ctx, glClearColor)(0.0, 0.0, 0.0, 1.0);
            GET_GLFUNC(ctx, glClear)(GL_COLOR_BUFFER_BIT);

            // [XXX] require when interacting with alpha blend thing.
            GET_GLFUNC(ctx, glEnable)(GL_BLEND);
            GET_GLFUNC(ctx, glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            return true;
        }

        bool RenderScene::add_layer(const GLRenderContext& ctx, const LayerContext& layer_ctx) {
            auto layer_type = static_cast<LayerType>(layer_ctx.type);
            switch (layer_type) {
                case LayerType::VIDEO: {
                    auto target = new VideoLayerTarget;
                    target->create(ctx, layer_ctx);
                    m_targets.push_back(target);
                    m_target_map.insert({layer_ctx.uuid, target});
                    break;
                }
                case LayerType::AUDIO: {
                    break;
                }
                case LayerType::TEXT: {
                    auto target = new TextLayerTarget;
                    target->create(ctx, layer_ctx);
                    m_targets.push_back(target);
                    m_target_map.insert({layer_ctx.uuid, target});
                    break;
                }
                case LayerType::IMAGE: {
                    auto target = new ImageLayerTarget;
                    target->create(ctx, layer_ctx);
                    m_targets.push_back(target);
                    m_target_map.insert({layer_ctx.uuid, target});
                    break;
                }
                default: {
                    AKLOG_ERROR("RenderScene::add_layer(): Invalid layer type '{}' found",
                                layer_type);
                    return false;
                }
            }
            return true;
        }
    }
}
