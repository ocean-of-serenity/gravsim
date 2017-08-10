
#version 450 core


layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;
layout (location = 2) in mat4 model;

out vec4 v_color;
out mat4 v_model;


void main() {
    gl_Position = position;
    v_color = color;
    v_model = model;
}

