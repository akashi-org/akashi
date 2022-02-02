#include "./mesh.h"

namespace akashi {
    namespace graphics {

        void BaseMesh::destroy_base() {
            glDeleteBuffers(1, &m_ibo);
            glDeleteVertexArrays(1, &m_vao);
            m_ibo_length = 0;
        }

        void BaseMesh::destroy() {
            this->destroy_inner();
            this->destroy_base();
        }

        void create_buffer(GLuint& buffer, GLenum target, void* data, size_t data_size,
                           GLenum usage) {
            glGenBuffers(1, &buffer);
            glBindBuffer(target, buffer);
            glBufferData(target, data_size, data, usage);
            glBindBuffer(target, 0);
        }

    }
}
