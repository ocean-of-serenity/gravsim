
#version 450 core


uniform uint active_buffers;

uniform mat4 projection;
uniform mat4 view;


struct Orb {
	vec4 location1;
	vec4 velocity1;
	vec4 location2;
	vec4 velocity2;
};


layout(std430, binding=0) buffer Orbs {
	Orb orbs[];
};


layout(triangles, equal_spacing) in;
in tcs {
    vec4 color;
    mat4 model;
    uint instance;
} in_[];

out tes {
    vec3 position;
    vec3 normal;
    vec4 color;
    uint instance;
} out_;


void main() {
    out_.instance = in_[0].instance;

	vec3 location = (active_buffers == 1 ? orbs[out_.instance].location2.xyz : orbs[out_.instance].location1.xyz);
	mat4 model = in_[0].model;
	model[3].xyz = location;

    vec3 p0 = gl_TessCoord.x * gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_TessCoord.y * gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_TessCoord.z * gl_in[2].gl_Position.xyz;
    vec4 position = vec4(normalize(p0 + p1 + p2), 1);

    out_.position = (model * position).xyz;
    out_.normal = normalize(out_.position - model[3].xyz);
    gl_Position = projection * (view * model) * position;

    vec4 c0 = gl_TessCoord.x * in_[0].color;
    vec4 c1 = gl_TessCoord.y * in_[1].color;
    vec4 c2 = gl_TessCoord.z * in_[2].color;
    out_.color = c0 + c1 + c2;
}

