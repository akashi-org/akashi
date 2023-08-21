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
#include <libakcore/color.h>
#include <libakeval/item.h>

namespace akashi {
    namespace graphics {

        namespace priv {
            static void init_renderer(const FBInfo& info, const std::array<float, 4>& bg_color) {
                glBindFramebuffer(GL_FRAMEBUFFER, info.fbo);
                glViewport(0.0, 0.0, info.width, info.height);
                glScissor(0.0, 0.0, info.width, info.height);

                glEnable(GL_SCISSOR_TEST);

                glClearColor(bg_color[0], bg_color[1], bg_color[2], bg_color[3]);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glDisable(GL_SCISSOR_TEST);

                glDisable(GL_MULTISAMPLE);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
        }

        RenderPlane::RenderPlane(OGLRenderContext& render_ctx, const core::PlaneContext& plane_ctx,
                                 const core::AtomStaticProfile& atom_static_profile)
            : m_plane_ctx(plane_ctx), m_atom_static_profile(atom_static_profile) {
            if (m_plane_ctx.level > 0) {
                m_base_layer = m_plane_ctx.base(render_ctx.eval_gctx(), m_plane_ctx);
                auto fb_size = m_base_layer.unit_layer_ctx.fb_size;

                if (!(m_fbo.create(fb_size[0], fb_size[1], render_ctx.msaa()))) {
                    AKLOG_ERRORN("Failed to create FBO");
                }

                ProjectionState proj_state;
                proj_state.video_width = fb_size[0];
                proj_state.video_height = fb_size[1];

                ViewState view_state;
                view_state.camera = glm::vec3(0, 0, 1);

                m_camera = core::make_owned<Camera>(proj_state, &view_state);
            }
        }

        void RenderPlane::destroy(const OGLRenderContext& ctx) {
            m_fbo.destroy();

            for (auto&& actor : m_actors) {
                if (actor) {
                    actor->destroy(ctx);
                    delete actor;
                }
            }
            m_actors.clear();
            m_actor_map.clear();
        }

        void RenderPlane::update(const core::PlaneContext& plane_ctx) { m_plane_ctx = plane_ctx; }

        bool RenderPlane::render(OGLRenderContext& render_ctx, const core::Rational& pts,
                                 const Stage& stage) {
            auto& cur_fbo = m_plane_ctx.level == 0 ? render_ctx.mut_fbo() : m_fbo;

            auto bg_color = m_plane_ctx.level == 0 ? m_atom_static_profile.bg_color
                                                   : m_base_layer.unit_layer_ctx.bg_color;
            std::array<float, 4> fb_bg_color = core::to_rgba_float(bg_color);
            priv::init_renderer(cur_fbo.info(), fb_bg_color);

            auto cur_layer_ctxs = m_plane_ctx.eval(render_ctx.eval_gctx(), m_plane_ctx);
            if (m_initial_render) {
                for (const auto& layer_ctx : cur_layer_ctxs) {
                    this->add_layer(render_ctx, layer_ctx);
                }
                m_initial_render = false;
            } else {
                for (const auto& layer_ctx : cur_layer_ctxs) {
                    auto it = m_actor_map.find(layer_ctx.uuid);
                    if (it != m_actor_map.end()) {
                        it->second->update_layer(layer_ctx);
                    }
                }
            }

            for (auto iter = m_actors.rbegin(), end = m_actors.rend(); iter != end; ++iter) {
                const auto& actor = *iter;
                if (!actor) {
                    continue;
                }
                const auto& layer_ctx = actor->get_layer_ctx();
                if (layer_ctx.display) {
                    if (static_cast<core::LayerType>(layer_ctx.type) == core::LayerType::UNIT) {
                        RenderPlane* render_plane = nullptr;
                        if (stage.find_render_plane(&render_plane, layer_ctx.uuid)) {
                            if (render_plane->m_fbo.initilized()) {
                                // lifetime?
                                actor->set_fbo(core::borrowed_ptr{&render_plane->m_fbo});
                            }
                        }
                    }
                    CHECK_AK_ERROR2(actor->render(render_ctx, pts,
                                                  m_camera ? *m_camera : *render_ctx.camera()));
                }
            }

            cur_fbo.resolve();

            return true;
        }

        bool RenderPlane::add_layer(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) {
            if (layer_ctx.type == static_cast<int>(core::LayerType::AUDIO)) {
                AKLOG_DEBUG("skip adding the audio layer: {}", layer_ctx.uuid);
                return false;
            }

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

        struct StageAux {};

        bool Stage::create(const OGLRenderContext& /* ctx */) {
            this->init_gl();
            return true;
        }

        bool Stage::destroy(const OGLRenderContext& ctx) {
            this->destroy_planes(ctx);

            if (m_aux) {
                delete m_aux;
            }
            m_aux = nullptr;
            return true;
        }

        bool Stage::render(OGLRenderContext& ctx, const RenderParams& params,
                           const core::FrameContext& frame_ctx) {
            // render planes to fbo
            CHECK_AK_ERROR2(this->render_planes(ctx, frame_ctx));

            // render fbo to the provided framebuffer
            priv::init_renderer({params.default_fb, params.screen_width, params.screen_height},
                                {0, 0, 0, 1});
            ctx.mut_fbo().render(ctx);

            return true;
        }

        bool Stage::encode_render(OGLRenderContext& ctx, const core::FrameContext& frame_ctx) {
            // render planes to fbo
            CHECK_AK_ERROR2(this->render_planes(ctx, frame_ctx));
            return true;
        }

        bool Stage::find_render_plane(RenderPlane** render_plane,
                                      const std::string& unit_uuid) const {
            auto it = m_plane_map.find(unit_uuid);
            if (it != m_plane_map.end()) {
                *render_plane = it->second;
                // [TODO] maybe we should check the validity of the render_plane
            }

            if (!(*render_plane)) {
                return false;
            }

            return true;
        }

        void Stage::init_gl() {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }

        bool Stage::render_planes(OGLRenderContext& ctx, const core::FrameContext& frame_ctx) {
            // [TODO] Apparently we need refactoring here
            // prepare planes
            {
                for (auto&& cur_plane : m_planes) {
                    if (cur_plane->plane_ctx().level > 0) {
                        cur_plane->set_defunct(true);
                    }
                }

                for (auto&& new_plane_ctx : frame_ctx.plane_ctxs) {
                    bool new_flg = true;
                    for (auto&& cur_plane : m_planes) {
                        if (new_plane_ctx.base_uuid == cur_plane->plane_ctx().base_uuid) {
                            // to be updated
                            cur_plane->set_defunct(false);
                            cur_plane->update(new_plane_ctx);
                            new_flg = false;
                            break;
                        }
                    }
                    if (new_flg) {
                        // newly added
                        this->add_plane(ctx, new_plane_ctx, frame_ctx.atom_static_profile);
                    }
                }

                // remove defunct planes
                decltype(m_planes) temp_planes;
                for (auto&& cur_plane : m_planes) {
                    if (cur_plane->is_defunct()) {
                        m_plane_map.erase(cur_plane->base_layer().uuid);
                        cur_plane->destroy(ctx);
                        delete cur_plane;
                    } else {
                        temp_planes.push_back(cur_plane);
                    }
                }
                m_planes.clear();
                m_planes = temp_planes;

                // sort planes
                std::sort(m_planes.begin(), m_planes.end(), [](RenderPlane* a, RenderPlane* b) {
                    return a->plane_ctx().level > b->plane_ctx().level;
                });
            }

            // render
            for (const auto& m_plane : m_planes) {
                CHECK_AK_ERROR2(m_plane->render(ctx, frame_ctx.pts, *this));
            }

            return true;
        }

        bool Stage::add_plane(OGLRenderContext& ctx, const core::PlaneContext& plane_ctx,
                              const core::AtomStaticProfile& atom_static_profile) {
            auto plane = new RenderPlane(ctx, plane_ctx, atom_static_profile);
            m_planes.push_back(plane);
            m_plane_map.insert({plane->base_layer().uuid, plane});

            return true;
        }

        void Stage::destroy_planes(const OGLRenderContext& ctx) {
            for (auto&& plane : m_planes) {
                if (plane) {
                    plane->destroy(ctx);
                    delete plane;
                }
            }
            m_planes.clear();
            m_plane_map.clear();
        }

    }
}
