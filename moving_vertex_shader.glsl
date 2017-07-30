
#version 450

uniform mat4 projection;
uniform mat4 view;
uniform float time;

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;
layout (location = 2) in mat4 model;

out vec4 v_color;


mat4 translate(in vec3 direction) {
    mat4 tm = mat4(1);
    tm[3] = vec4(direction, 1);
    return tm;
}

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

vec3 perp_plane(in vec3 direction) {
    if( direction.x != 0 )
        return vec3((-direction.y - direction.z) / direction.x, 1, 1);
    else if( direction.y != 0 )
        return vec3(1, (-direction.x - direction.z) / direction.y, 1);
    else if( direction.z != 0 )
        return vec3(1, 1, (-direction.x - direction.y) / direction.z);
    else
        return direction;
}


void main() {
    vec3 axis = perp_plane(vec3(model[3]));
    mat4 mrvp = projection * view * rotate(time / 2, axis) * model;
    gl_Position = mrvp * position;
    v_color = color;
}

