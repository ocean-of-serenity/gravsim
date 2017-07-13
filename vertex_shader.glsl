
#version 450

in vec3 position;
in vec4 color;

out vec4 v_color;

uniform float time;

void main() {
    gl_Position = vec4(
			position.x + cos(time),
			position.y + sin(time),
			position.z,
			1.0
	);
    v_color = color;
}
