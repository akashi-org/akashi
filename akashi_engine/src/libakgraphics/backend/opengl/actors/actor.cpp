#include "./actor.h"

#include "./image_actor.h"
#include "./video_actor.h"
#include "./effect_actor.h"
#include "./text_actor.h"
#include "./shape_actor.h"

#include <libakcore/element.h>
#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        Actor* create_actor(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) {
            auto layer_type = static_cast<LayerType>(layer_ctx.type);

            Actor* actor = nullptr;
            switch (layer_type) {
                case LayerType::VIDEO: {
                    actor = new VideoActor;
                    break;
                }
                case LayerType::AUDIO: {
                    AKLOG_WARNN("Not implemented");
                    return nullptr;
                }
                case LayerType::TEXT: {
                    actor = new TextActor;
                    break;
                }
                case LayerType::IMAGE: {
                    actor = new ImageActor;
                    break;
                }
                case LayerType::EFFECT: {
                    actor = new EffectActor;
                    break;
                }
                case LayerType::SHAPE: {
                    actor = new ShapeActor;
                    break;
                }
                default: {
                    AKLOG_ERROR("Invalid layer type '{}' found", layer_type);
                    return nullptr;
                }
            }

            if (!actor->create(ctx, layer_ctx)) {
                actor->destroy(ctx);
                delete actor;
                return nullptr;
            }
            return actor;
        }

    }

}
