
#version 450


layout(local_size_x=256) in;

uniform float distance;

layout(std430, binding=1) buffer Models {
    mat4 models[];
};

layout(std430, binding=2) buffer Axiis {
    vec4 axiis[];
};


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


void main() {
    uint id = gl_GlobalInvocationID.x;
    if( id != 0 ) {
        vec4 position = vec4(models[id][3].xyz, 1);
        mat4 rotation = rotate(distance, axiis[id].xyz);
        vec4 new_position = rotation * position;
        models[id][3].xyz = new_position.xyz;
    }
}


