
#version 450 core


layout(std140, binding=1) uniform Light {
    vec3 l_position;
    vec3 l_color;
};

layout(std140, binding=2) uniform Material {
    float m_ambient;
    float m_diffuse;
    float m_specular;
};

layout(std140, binding=3) uniform Camera {
    vec3 c_root;
    vec3 c_watch;
};

in vec3 te_position;
in vec3 te_normal;
in vec4 te_color;

out vec4 f_color;


void main() {
    vec3 ambient = l_color * m_ambient;

    vec3 light_direction = normalize(l_position - te_position);
    vec3 diffuse = l_color * max(dot(te_normal, light_direction), 0) * m_diffuse;

    vec3 view_direction = normalize(c_root - te_position);
    vec3 reflect_direction = reflect(-light_direction, te_normal);
    vec3 specular = l_color * pow(max(dot(view_direction, reflect_direction), 0), 32) * m_specular;

    f_color = vec4((ambient + diffuse + specular) * te_color.rgb, te_color.a);
}

