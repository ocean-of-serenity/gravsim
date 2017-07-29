
#ifndef OPENGL_TEST_SPHERE_H
#define OPENGL_TEST_SPHERE_H


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "base.h"


size_t sphere_vertexbuffer_size(const size_t divisor) {
    if( divisor == 0 ) {
        return 0;
    }
    else {
        return 2 + 4 * (divisor + (divisor - 1) * divisor);
    }
}

void sphere_vertex_positions(Vertex* const vertices, const size_t divisor) {
    if( divisor == 0 ) {
        return;
    }
    else {
        size_t index = 0;

        vertices[index++].position = glm::vec3(0, 1, 0);

        const float top_angle_segment = -90.0f / divisor;
        for( size_t upper_lat = 1; upper_lat < divisor; upper_lat++ ) {
            const glm::mat4 rot_z = glm::rotate(
                    glm::mat4(1),
                    glm::radians(top_angle_segment * upper_lat),
                    glm::vec3(0, 0, 1)
            );
            const glm::vec3 lat_start = glm::mat3(rot_z) * glm::vec3(0, 1, 0);

            const float circ_angle_segment = 360.0f / (4 * upper_lat);
            for(
                    size_t long_verts = 0;
                    long_verts < 4 * upper_lat;
                    long_verts++
            ) {
                const glm::mat4 rot_y = glm::rotate(
                        glm::mat4(1),
                        glm::radians(circ_angle_segment * long_verts),
                        glm::vec3(0, 1, 0)
                );
                vertices[index++].position = glm::mat3(rot_y) * lat_start;
            }
        }

        const glm::vec3 eq_start(1, 0, 0);
        const float eq_circ_angle_segment = 360.0f / (4 * divisor);
        for( size_t long_verts = 0; long_verts < 4 * divisor; long_verts++ ) {
            const glm::mat4 rot_y = glm::rotate(
                    glm::mat4(1),
                    glm::radians(eq_circ_angle_segment * long_verts),
                    glm::vec3(0, 1, 0)
            );
            vertices[index++].position = glm::mat3(rot_y) * eq_start;
        }

        const float bottom_angle_segment = 90.0f / divisor;
        for( size_t lower_lat = divisor - 1; lower_lat > 0; lower_lat-- ) {
            const glm::mat4 rot_z = glm::rotate(
                    glm::mat4(1),
                    glm::radians(bottom_angle_segment * lower_lat),
                    glm::vec3(0, 0, 1)
            );
            const glm::vec3 lat_start = glm::mat3(rot_z) * glm::vec3(0, -1, 0);

            const float circ_angle_segment = 360.0f / (4 * lower_lat);
            for(
                    size_t long_verts = 0;
                    long_verts < 4 * lower_lat;
                    long_verts++
            ) {
                const glm::mat4 rot_y = glm::rotate(
                        glm::mat4(1),
                        glm::radians(circ_angle_segment * long_verts),
                        glm::vec3(0, 1, 0)
                );
                vertices[index++].position = glm::mat3(rot_y) * lat_start;
            }
        }

        vertices[index++].position = glm::vec3(0, -1, 0);

        return;
    }
}


size_t sphere_elementbuffer_size(const size_t divisor) {
    if( divisor == 0 ) {
        return 0;
    }
    else {
        return
            13
            + 8 * (divisor * (divisor + 1) + divisor - 3)
            + 8 * (divisor - 1)
        ;
    }
}

void sphere_elements(GLuint* const elements, const size_t divisor) {
    if( divisor == 0 ) {
        return;
    }
    else {
        const size_t n_elements = sphere_vertexbuffer_size(divisor);

        size_t index = 0;
        
        elements[index++] = 0;
        for( GLuint e = 1; e < 5; e++ ) {
            elements[index++] = e;
        }
        elements[index++] = 1;

        elements[index++] = RESTART_INDEX;

        elements[index++] = (n_elements - 1) - 0;
        for( GLuint e = 1; e < 5; e++ ) {
            elements[index++] = (n_elements - 1) - e;
        }
        elements[index++] = (n_elements - 1) - 1;

        for(
                GLuint
                    r = 1,
                    top_base = 1 + (r - 1) * 4,
                    bot_base = 1 + r * 4
                ;
                r < divisor;
                top_base += r * 4, bot_base += (r + 1) * 4, r++
        ) {
            const GLuint top_step = r;
            const GLuint bot_step = r + 1;

            for( GLuint f = 0; f < 4; f++ ) {
                const GLuint top_f_base = top_base + f * top_step;
                const GLuint bot_f_base = bot_base + f * bot_step;

                GLuint e = 0;
                for( ; e <= divisor; e++ ) {
                    elements[index++] = bot_f_base + e;
                    elements[index++] = top_f_base + e;
                }
                elements[index++] = bot_f_base + e;
                elements[index++] = RESTART_INDEX;
            }
        }

        for(
                GLuint
                    r = 1,
                    bot_base = (n_elements - 1) - 1 - (r - 1) * 4,
                    top_base = (n_elements - 1) - 1 - r * 4
                ;
                r < divisor;
                bot_base -= r * 4, top_base -= (r + 1) * 4, r++
        ) {
            const GLuint bot_step = r;
            const GLuint top_step = r + 1;

            for( GLuint f = 0; f < 4; f++ ) {
                const GLuint bot_f_base = bot_base - f * bot_step;
                const GLuint top_f_base = top_base - f * top_step;

                GLuint e = 0;
                for( ; e <= divisor; e++ ) {
                    elements[index++] = top_f_base - e;
                    elements[index++] = bot_f_base - e;
                }
                elements[index++] = top_f_base - e;
                elements[index++] = RESTART_INDEX;
            }
        }
    }
}


size_t sphere_elementbuffer_partitions_size(const size_t divisor) {
    return (divisor > 1) ? 2 : divisor;
}

void sphere_elementbuffer_partitions(
        EBOPartitionTable* const ebopt,
        const VertexArray* const va,
        const size_t divisor
) {
    if( ebopt->size == 0 or divisor == 0 ) {
        return;
    }
    else {
        ebopt->table[0] = EBOPartition(GL_TRIANGLE_FAN, 0, 13);

        if( ebopt->size > 1 and divisor > 1 ) {
            ebopt->table[1] = EBOPartition(
                    GL_TRIANGLE_STRIP,
                    14,
                    va->ebo_size - 14
            );
        }
    }
}


#endif //OPENGL_TEST_SPHERE_H

