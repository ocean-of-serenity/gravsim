
#version 450

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

in vec4 position;
in vec4 color;

out vec4 v_color;

void main() {
    gl_Position = projection * view * model * position;
    v_color = color;
}

