
#version 450 core


layout (location = 0) in vec4 position;
layout (location = 2) in mat4 model;

out mat4 v_model;


void main() {
    gl_Position = position;
    v_model = model;
}

