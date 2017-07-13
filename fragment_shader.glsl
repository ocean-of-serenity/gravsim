
#version 450

in vec4 v_color;

uniform float time;

void main() {
    gl_FragColor = vec4(abs(cos(time)) * v_color.xyz, v_color.w);
}
