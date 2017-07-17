
#version 450

in vec4 position;
in vec4 color;

out vec4 v_color;

mat4 rotate_x(in float rad) {
    return mat4(
                    1,         0,         0,         0,
                    0,  cos(rad),  sin(rad),         0,
                    0, -sin(rad),  cos(rad),         0,
                    0,         0,         0,         1
    );
}

mat4 rotate_y(in float rad) {
    return mat4(
             cos(rad),         0, -sin(rad),         0,
                    0,         1,         0,         0,
             sin(rad),         0,  cos(rad),         0,
                    0,         0,         0,         1
    );
}

mat4 rotate_z(in float rad) {
    return mat4(
             cos(rad),  sin(rad),         0,         0,
            -sin(rad),  cos(rad),         0,         0,
                    0,         0,         1,         0,
                    0,         0,         0,         1
    );
}

void main() {
    gl_Position = (rotate_z(radians(-15)) * rotate_x(radians(30)) * rotate_y(radians(-15))) * position;
    v_color = color;
}

