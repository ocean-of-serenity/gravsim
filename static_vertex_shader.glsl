
#version 450


uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;
layout (location = 2) in mat4 model;

out vec4 v_color;


mat4 rotate(in float angle, in vec3 axis) {
    vec3 u = normalize(axis);

    mat3 cu = mat3(
               0, -u.z,  u.y,
             u.z,    0, -u.x,
            -u.y,  u.x,    0
    );

    mat3 opu = outerProduct(u, u);

    mat3 rm3 = cos(angle) * mat3(1) + sin(angle) * cu + (1 - cos(angle)) * opu;

    return mat4(
            vec4(rm3[0], 0),
            vec4(rm3[1], 0),
            vec4(rm3[2], 0),
            vec4(0, 0, 0, 1)
    );

    return mat4(1);
}

mat4 scale(in float scalar) {
    mat3 sm = scalar * mat3(1);
    return mat4(
            vec4(sm[0], 0),
            vec4(sm[1], 0),
            vec4(sm[2], 0),
            vec4(0, 0, 0, 1)
    );
}

mat4 translate(in vec3 direction) {
    mat4 tm = mat4(1);
    tm[3] = vec4(direction, 1);
    return tm;
}

void main() {
    mat4 mvp = projection * view * model;
    gl_Position = mvp * position;
    v_color = color;
}

