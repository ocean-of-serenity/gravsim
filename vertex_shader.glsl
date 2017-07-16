
#version 450

uniform float time;

in vec3 position;
in vec4 color;

out vec4 v_color;

void main() {
    mat2 trans = mat2(
             cos(time), sin(time),
            -sin(time), cos(time)
    );
    gl_Position = vec4(trans * position.xy, position.z, 1.0);
    v_color = color;
}

