
#version 450

in vec4 v_color;

uniform float time;

void main() {
    gl_FragColor = vec4(v_color.xyz * abs(sin(time)), 1.0);
}
