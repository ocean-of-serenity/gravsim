
#version 450 core


layout(std140, binding=1) uniform Light {
    vec3 position;
    vec3 color;
} light;

uniform float ambient_part;

uniform vec3 camera_location;

in tes {
    vec3 position;
    vec3 normal;
    vec4 color;
    flat uint instance;
} in_;

layout(location=0) out vec4 color;


void main() {
    if( in_.instance == 0 ) {
        color = in_.color;
    }
    else {
        vec3 ambient = light.color * ambient_part;

        vec3 light_direction = normalize(light.position - in_.position);

        vec3 diffuse =  light.color * max(dot(in_.normal, light_direction), 0) * 0.9;

        vec3 view_direction = normalize(camera_location - in_.position);
        vec3 reflect_direction = reflect(-light_direction, in_.normal);

        vec3 specular = light.color * pow(max(dot(view_direction, reflect_direction), 0), 16) * 1.3;

        color = vec4((ambient + diffuse) * in_.color.rgb + specular, in_.color.a);
    }
}

