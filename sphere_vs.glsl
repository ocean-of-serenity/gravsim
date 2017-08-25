
#version 450 core


layout(location=0) in vec4 position;
layout(location=1) in vec4 color;
layout(location=2) in mat4 model;

out vs {
    vec4 color;
    mat4 model;
} out_;


void main() {
    gl_Position = position;
    out_.color = color;
    out_.model = model;
}

