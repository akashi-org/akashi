#pragma once

#include <array>
#include <cstddef>

#include <glm/glm.hpp>

namespace akashi {
    namespace graphics {

        class Grid final {
            struct Pass;

          public:
            enum class Type { X = 0, Z, LENGTH };
            struct State {
                Grid::Type type;
                std::array<float, 4> base_color = {0.6, 0.6, 0.6, 1};
                std::array<float, 4> main_color = {1.0, 1.0, 1.0, 1};
                size_t line_counts = 0;
            };

          public:
            explicit Grid(Grid::State* state = nullptr);
            virtual ~Grid();
            void update();
            void render(const glm::mat4& pv);

          private:
            bool init_vertices();

          private:
            Pass* m_pass = nullptr;
            Grid::State m_state;
        };
    }
}
