
#ifndef OPENGL_TEST_BASE_H
#define OPENGL_TEST_BASE_H


#include <vector>

#include <glm/glm.hpp>


const GLuint RESTART_INDEX = std::numeric_limits<GLuint>::max();


struct BufferMapping {
    const GLuint bo;
    const void* const buffer;

    BufferMapping() = delete;
    BufferMapping(const BufferMapping&) = delete;
    BufferMapping(const BufferMapping&&) = delete;
    BufferMapping& operator=(const BufferMapping&) = delete;
    BufferMapping& operator=(const BufferMapping&&) = delete;
    
    explicit BufferMapping(const GLuint bo)
        :   bo(bo),
            buffer(glMapNamedBuffer(bo, GL_WRITE_ONLY))
    {}

    ~BufferMapping() {
        glUnmapNamedBuffer(this->bo);
    }
};


struct Vertex {
    glm::vec3 position;
    glm::u8vec4 color;    
};


struct EBOPartition {
    const GLenum mode;
    const size_t offset;
    const size_t size;
}; 


struct Buffer {
    const GLuint bo;
    const size_t stride;
    const size_t size;

    Buffer() = delete;
    Buffer(const Buffer&) = delete;
    Buffer(const Buffer&&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(const Buffer&&) = delete;

    explicit Buffer(
            const size_t stride,
            const size_t size
    )
        :   bo(Buffer::create()),
            stride(stride),
            size(size)
    {
        glNamedBufferStorage(
                this->bo,
                this-> stride * this->size,
                NULL,
                GL_MAP_WRITE_BIT
        );
    }

    ~Buffer() {
        Buffer::discard(this->bo);
    }


    std::unique_ptr<BufferMapping> map() const {
        return std::make_unique<BufferMapping>(this->bo);
    }


    static GLuint create() {
        GLuint b_name;
        glCreateBuffers(1, &b_name);
        return b_name;
    }

    static void discard(const GLuint b_name) {
        glDeleteBuffers(1, &b_name);
    }
};


struct VertexArray {
    const GLuint vao;
    const Buffer vb;
    const Buffer eb;
    const Buffer imb;
    const std::vector<EBOPartition> p_table;

    VertexArray() = delete;
    VertexArray(const VertexArray&) = delete;
    VertexArray(VertexArray&&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray& operator=(VertexArray&&) = delete;

    explicit VertexArray(
            const size_t vb_size,
            const size_t eb_size,
            const size_t imb_size,
            const std::vector<EBOPartition>& p_table
    )
        :   vao(VertexArray::create()),
            vb(sizeof(Vertex), vb_size),
            eb(sizeof(GLuint), eb_size),
            imb(sizeof(glm::mat4), imb_size),
            p_table(p_table)
    {
        glVertexArrayVertexBuffer(
                this->vao,
                0,
                this->vb.bo,
                0,
                sizeof(Vertex)
        );
        glEnableVertexArrayAttrib(this->vao, 0);
        glVertexArrayAttribBinding(this->vao, 0, 0);
        glVertexArrayAttribFormat(
                this->vao,
                0,
                3,
                GL_FLOAT,
                GL_FALSE,
                offsetof(Vertex, position)
        );
        glEnableVertexArrayAttrib(this->vao, 1);
        glVertexArrayAttribBinding(this->vao, 1, 0);
        glVertexArrayAttribFormat(
                this->vao,
                1,
                4,
                GL_UNSIGNED_BYTE,
                GL_TRUE,
                offsetof(Vertex, color)
        );

        glVertexArrayElementBuffer(this->vao, this->eb.bo);

        glVertexArrayVertexBuffer(
                this->vao,
                1,
                this->imb.bo,
                0,
                sizeof(glm::mat4)
        );
        glVertexArrayBindingDivisor(this->vao, 1, 1);
        for( size_t i = 0; i < 4; i++ ) {
            glEnableVertexArrayAttrib(this->vao, 2 + i);
            glVertexArrayAttribBinding(this->vao, 2 + i, 1);
            glVertexArrayAttribFormat(
                    this->vao,
                    2 + i,
                    4,
                    GL_FLOAT,
                    GL_FALSE,
                    (sizeof(glm::mat4) / 4) * i
            );
        }
    }

    ~VertexArray() {
        VertexArray::discard(this->vao);
    }


    void draw() {
        glBindVertexArray(this->vao);
        for( size_t i = 0; i < this->p_table.size(); i++ ) {
            glDrawElementsInstanced(
                    this->p_table[i].mode,
                    this->p_table[i].size,
                    GL_UNSIGNED_INT,
                    (void*) (this->p_table[i].offset * sizeof(GLuint)),
                    this->imb.size
            );
        }
        glBindVertexArray(0);
    }


    static GLuint create() {
        GLuint va_name;
        glCreateVertexArrays(1, &va_name);
        return va_name;
    }

    void discard(const GLuint va_name) {
        glDeleteVertexArrays(1, &va_name);
    }

};


#endif //OPENGL_TEST_BASE_H

