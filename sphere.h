
#ifndef OPENGL_TEST_SPHERE_H
#define OPENGL_TEST_SPHERE_H


#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "base.h"


size_t sphere_vertexbuffer_size(const size_t divisor) {
    if( divisor == 0 ) {
        return 0;
    }
    else {
        return 2 + 4 * (divisor + divisor * divisor);
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

        const size_t upper_lat_end = index;
        for( size_t i = 0; i < upper_lat_end; i++ ) {
            const glm::vec3 temp = vertices[i].position;
            vertices[index++].position = glm::vec3(temp.x, -temp.y, temp.z);
        }

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
        const size_t n_vertices = sphere_vertexbuffer_size(divisor);
        const size_t second_half = n_vertices / 2;
        const size_t n_elements = sphere_elementbuffer_size(divisor);

        size_t index = 0;
        
        elements[index++] = 0;
        for( GLuint e = 1; e <= 4; e++ ) {
            elements[index++] = e;
        }
        elements[index++] = 1;

        elements[index++] = RESTART_INDEX;

        elements[index++] = second_half + 0;
        for( GLuint e = 1; e <= 4; e++ ) {
            elements[index++] = second_half + e;
        }
        elements[index++] = second_half + 1;

        {
            GLuint top_base = 1;
            GLuint bot_base = 1;
            for( GLuint row = 1; row < divisor; row++ ) {
                const GLuint top_step = row;
                const GLuint bot_step = row + 1;

                top_base += (row - 1) * 4;
                bot_base += row * 4;

                for( GLuint face = 0; face < 4; face++ ) {
                    const GLuint face_top_base = top_base + face * top_step;
                    const GLuint face_bot_base = bot_base + face * bot_step;

                    const GLuint n_face_squares = face > 2 ? row - 1 : row;

                    elements[index++] = face_bot_base;
                    for( GLuint el = 0; el <= n_face_squares; el++ ) {
                        elements[index++] = face_top_base + el;
                        elements[index++] = face_bot_base + el + 1;
                    }

                    if( face > 2 ) {
                        elements[index++] = top_base;
                        elements[index++] = bot_base;
                    }

                    elements[index++] = RESTART_INDEX;
                }
            }
        }

        {
            GLuint top_base = second_half + 1;
            GLuint bot_base = second_half + 1;
            for( GLuint row = 1; row < divisor; row++ ) {
                const GLuint top_step = row;
                const GLuint bot_step = row + 1;

                top_base += (row - 1) * 4;
                bot_base += row * 4;

                for( GLuint face = 0; face < 4; face++ ) {
                    const GLuint face_top_base = top_base + face * top_step;
                    const GLuint face_bot_base = bot_base + face * bot_step;

                    const GLuint n_face_squares = face > 2 ? row - 1 : row;

                    elements[index++] = face_bot_base;
                    for( GLuint el = 0; el <= n_face_squares; el++ ) {
                        elements[index++] = face_top_base + el;
                        elements[index++] = face_bot_base + el + 1;
                    }

                    if( face > 2 ) {
                        elements[index++] = top_base;
                        elements[index++] = bot_base;
                    }

                    elements[index++] = RESTART_INDEX;
                }
            }
        }

        for( ; index < n_elements; index++ ) {
            elements[index] = RESTART_INDEX;
        }
    }
}


const std::vector<EBOPartition> sphere_elementbuffer_partitions(
        const size_t divisor
) {
    if( divisor == 0 ) {
        return {};
    }
    else if( divisor == 1 ) {
        return {{GL_TRIANGLE_FAN, 0, 13}};
    }
    else {
        return {
            {GL_TRIANGLE_FAN, 0, 13},
            {GL_TRIANGLE_STRIP, 13, sphere_elementbuffer_size(divisor) - 13}
        };
    }
}


#endif //OPENGL_TEST_SPHERE_H

