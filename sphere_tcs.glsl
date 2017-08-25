
#version 450 core


in vs {
    vec4 color;
    mat4 model;
} in_[];

layout(vertices=3) out;
out tcs {
    vec4 color;
    mat4 model;
} out_[];


void main() {
    gl_TessLevelInner[0] = 24;
    gl_TessLevelOuter[gl_InvocationID] = 23;

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    out_[gl_InvocationID].color = in_[gl_InvocationID].color;
    out_[gl_InvocationID].model = in_[gl_InvocationID].model;
}

