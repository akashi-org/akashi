#include "./actor.h"

#include "./image_actor.h"

#include <libakcore/element.h>
#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        Actor* create_actor(const OGLRenderContext& ctx, const core::LayerContext& layer_ctx) {
            auto layer_type = static_cast<LayerType>(layer_ctx.type);
            switch (layer_type) {
                case LayerType::VIDEO: {
                    AKLOG_WARNN("Not implemented");
                    return nullptr;
                }
                case LayerType::AUDIO: {
                    AKLOG_WARNN("Not implemented");
                    return nullptr;
                }
                case LayerType::TEXT: {
                    AKLOG_WARNN("Not implemented");
                    return nullptr;
                }
                case LayerType::IMAGE: {
                    auto actor = new ImageActor;
                    if (!actor->create(ctx, layer_ctx)) {
                        actor->destroy(ctx);
                        delete actor;
                        return nullptr;
                    }
                    return actor;
                }
                case LayerType::EFFECT: {
                    AKLOG_WARNN("Not implemented");
                    return nullptr;
                }
                default: {
                    AKLOG_ERROR("Invalid layer type '{}' found", layer_type);
                    return nullptr;
                }
            }
        }

    }

}
