
#version 450 core


layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;
layout (location = 2) in mat4 model;

out vec4 v_color;


void main() {
    gl_Position = model * position;
    v_color = color;
}

