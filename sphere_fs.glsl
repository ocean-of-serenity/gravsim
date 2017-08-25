
#version 450 core


layout(std140, binding=1) uniform Light {
    vec3 position;
    vec3 color;
} light;

uniform float ambient_part;
uniform float diffuse_part;
uniform float specular_part;

uniform vec3 camera_location;

in tes {
    vec3 position;
    vec3 normal;
    vec4 color;
} in_;

layout(location=0) out vec4 color;


void main() {
    vec3 ambient = light.color * ambient_part;

    vec3 light_direction = normalize(light.position - in_.position);
    vec3 diffuse =  light.color *
                    max(dot(in_.normal, light_direction), 0) *
                    diffuse_part;

    vec3 view_direction = normalize(camera_location - in_.position);
    vec3 reflect_direction = reflect(-light_direction, in_.normal);
    vec3 specular = light.color *
                    pow(max(dot(view_direction, reflect_direction), 0), 32) *
                    specular_part;

    color = vec4((ambient + diffuse + specular) * in_.color.rgb, in_.color.a);
}

