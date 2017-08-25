
#version 450 core

in vs {
    vec4 color;
} in_;

layout(location=0) out vec4 color;


void main() {
    color = in_.color;
}

