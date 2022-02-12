#pragma once

#include "../core/glc.h"

#include <cstddef>

namespace akashi {
    namespace graphics {

        class BaseMesh {
          public:
            virtual ~BaseMesh() { this->destroy_base(); };

            virtual GLuint vao() const { return m_vao; }

            virtual GLuint ibo() const { return m_ibo; }

            virtual size_t ibo_length() const { return m_ibo_length; }

            void destroy();

          protected:
            virtual void destroy_inner(){};

          private:
            void destroy_base();

          protected:
            GLuint m_vao;
            GLuint m_ibo;
            size_t m_ibo_length = 0;
        };

        void create_buffer(GLuint& buffer, GLenum target, void* data, size_t data_size,
                           GLenum usage = GL_STATIC_DRAW);

    }
}
