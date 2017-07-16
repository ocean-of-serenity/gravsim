
#version 450

uniform float time;

in vec4 v_color;

out vec4 f_color;

void main() {
    f_color = vec4(
            v_color.x,
            v_color.y * abs(sin(time)),
            v_color.z,
            1.0
    );
}

