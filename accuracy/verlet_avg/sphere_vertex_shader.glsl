
#version 450 core


uniform int active_buffer;


layout(location=0) in vec4 position;
layout(location=1) in vec4 color;
layout(location=2) in mat4 model1;
layout(location=3) in mat4 model2;

out vs {
    vec4 color;
    mat4 model;
    uint instance;
} out_;


void main() {
    gl_Position = position;
    out_.color = color;
    out_.model = (active_buffer == 1 ? model2 : model1);
    out_.instance = gl_InstanceID;
}

