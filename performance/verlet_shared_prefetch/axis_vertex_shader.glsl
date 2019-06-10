
#version 450 core

uniform mat4 projection;
uniform mat4 view;

layout(location=0) in vec4 position;
layout(location=1) in vec4 color;
layout(location=2) in mat4 model;

out vs {
    vec4 color;
} out_;


void main() {
    gl_Position = projection * (view * model) * position;
    out_.color = color;
}

