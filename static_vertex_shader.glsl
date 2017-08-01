
#version 450


uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;
layout (location = 2) in mat4 model;

out vec4 v_color;


void main() {
    mat4 mvp = projection * view * model;
    gl_Position = mvp * position;
    v_color = color;
}

