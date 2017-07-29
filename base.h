
#ifndef OPENGL_TEST_BASE_H
#define OPENGL_TEST_BASE_H


#include <glm/glm.hpp>


const GLuint RESTART_INDEX = std::numeric_limits<GLuint>::max();


GLuint create_vertex_array() {
    GLuint vao;
    glCreateVertexArrays(1, &vao);
    return vao;
}

void delete_vertex_array(const GLuint vao) {
    glDeleteVertexArrays(1, &vao);
}


GLuint create_buffer() {
    GLuint bo;
    glCreateBuffers(1, &bo);
    return bo;
}

void delete_buffer(const GLuint bo) {
    glDeleteBuffers(1, &bo);
}


struct BufferMapping {
    const GLuint bo;
    const GLenum access;
    const void* const buffer;

    BufferMapping() = delete;
    BufferMapping(const BufferMapping&) = delete;
    BufferMapping(const BufferMapping&&) = delete;
    BufferMapping& operator=(const BufferMapping&) = delete;
    BufferMapping& operator=(const BufferMapping&&) = delete;
    
    BufferMapping(const GLuint bo, const GLenum access)
        :   bo(bo),
            access(access),
            buffer(glMapNamedBuffer(bo, access))
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
    GLenum mode;
    size_t offset;
    size_t size;

    EBOPartition() = default;

    EBOPartition(
            const GLenum mode,
            const size_t offset,
            const size_t size
    )
        :   mode(mode),
            offset(offset),
            size(size)
    {}
}; 


struct EBOPartitionTable {
    EBOPartition* const table;
    const size_t size;

    EBOPartitionTable() = delete;
    EBOPartitionTable(const EBOPartitionTable&) = delete;
    EBOPartitionTable(const EBOPartitionTable&&) = delete;
    EBOPartitionTable& operator=(const EBOPartitionTable&) = delete;
    EBOPartitionTable& operator=(const EBOPartitionTable&&) = delete;

    EBOPartitionTable(const size_t size)
        :   table(new EBOPartition[size]),
            size(size)
    {}

    ~EBOPartitionTable() {
        delete[] table;
    }
};


struct VertexArray {
    const GLuint vao;
    const GLuint vbo;
    const GLuint ebo;
    const GLuint imbo;
    const size_t vbo_size;
    const size_t ebo_size;
    const size_t imbo_size;

    VertexArray() = delete;
    VertexArray(const VertexArray&) = delete;
    VertexArray(const VertexArray&&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&&) = delete;

    VertexArray(
            const size_t vbo_size,
            const size_t ebo_size,
            const size_t imbo_size
    )
        :   vao(create_vertex_array()),
            vbo(create_buffer()),
            ebo(create_buffer()),
            imbo(create_buffer()),
            vbo_size(vbo_size),
            ebo_size(ebo_size),
            imbo_size(imbo_size)
    {
        glNamedBufferStorage(
                this->vbo,
                sizeof(Vertex) * this->vbo_size,
                NULL,
                GL_MAP_WRITE_BIT
        );
        glNamedBufferStorage(
                this->ebo,
                sizeof(GLuint) * this->ebo_size,
                NULL,
                GL_MAP_WRITE_BIT
        );
        glNamedBufferStorage(
                this->imbo,
                sizeof(glm::mat4) * this->imbo_size,
                NULL,
                GL_MAP_WRITE_BIT
        );

        glBindVertexArray(this->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
                0,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                (void*) offsetof(Vertex, position)
        );
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
                1,
                4,
                GL_UNSIGNED_BYTE,
                GL_TRUE,
                sizeof(Vertex),
                (void*) offsetof(Vertex, color)
        );

        glBindBuffer(GL_ARRAY_BUFFER, this->imbo);
        for( size_t i = 0; i < 4; i++ ) {
            glEnableVertexAttribArray(2 + i);
            glVertexAttribPointer(
                    2 + i,
                    4,
                    GL_FLOAT,
                    GL_FALSE,
                    sizeof(glm::mat4),
                    (void*) ((sizeof(glm::mat4) / 4) * i)
            );
            glVertexAttribDivisor(2 + i, 1);
        }

        glBindVertexArray(0); 
    }

    ~VertexArray() {
        delete_buffer(this->imbo);
        delete_buffer(this->ebo);
        delete_buffer(this->vbo);
        delete_vertex_array(this->vao);
    }
};


void draw_elements(
        const VertexArray* const va,
        const EBOPartitionTable* const ebopt
) {
    glBindVertexArray(va->vao);
    for( size_t i = 0; i < ebopt->size; i++ ) {
        glDrawElementsInstanced(
                ebopt->table[i].mode,
                ebopt->table[i].size,
                GL_UNSIGNED_INT,
                (void*) ebopt->table[i].offset,
                va->imbo_size
        );
    }
    glBindVertexArray(0);
}


#endif //OPENGL_TEST_BASE_H

