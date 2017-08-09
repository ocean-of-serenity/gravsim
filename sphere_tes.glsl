
#version 450 core


layout(triangles, equal_spacing) in;


in mat4 tc_model[];


void main() {
    vec3 p0 = gl_TessCoord.x * vec3(gl_in[0].gl_Position);
    vec3 p1 = gl_TessCoord.y * vec3(gl_in[1].gl_Position);
    vec3 p2 = gl_TessCoord.z * vec3(gl_in[2].gl_Position);
    gl_Position = tc_model[0] * vec4(normalize(p0 + p1 + p2), 1);
}

