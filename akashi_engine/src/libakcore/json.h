#pragma once

#include "./element.h"
#include "./common.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>

namespace akashi {
    namespace core {

        // --------------------------- [serializer/deserializer] ---------------------------

        void from_json(const nlohmann::json& j, Fraction& entry);

        void from_json(const nlohmann::json& j, Style& entry);

        void from_json(const nlohmann::json& j, LayerContext& entry);

        void from_json(const nlohmann::json& j, FrameContext& entry);

        void from_json(const nlohmann::json& j, LayerProfile& entry);

        void from_json(const nlohmann::json& j, AtomProfile& entry);

        void from_json(const nlohmann::json& j, RenderProfile& entry);

        void from_json(const nlohmann::json& j, RenderContext& entry);

        // --------------------------- [utilities] ---------------------------

        void parse_render_ctx(nlohmann::json& j, RenderContext& render_ctx);

        void parse_render_prof(nlohmann::json& j, RenderProfile& render_prof);

    }
}
