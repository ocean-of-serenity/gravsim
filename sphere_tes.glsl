
#version 450 core


uniform mat4 view_projection;

layout(triangles, equal_spacing) in;
in vec4 tc_color[];
in mat4 tc_model[];

out vec3 te_position;
out vec3 te_normal;
out vec4 te_color;


void main() {
    vec3 p0 = gl_TessCoord.x * gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_TessCoord.y * gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_TessCoord.z * gl_in[2].gl_Position.xyz;
    vec4 mv_pos = tc_model[0] * vec4(normalize(p0 + p1 + p2), 1);
    te_position = mv_pos.xyz;
    te_normal = normalize(te_position - tc_model[0][3].xyz);
    gl_Position = view_projection * mv_pos;

    vec4 c0 = gl_TessCoord.x * tc_color[0];
    vec4 c1 = gl_TessCoord.y * tc_color[1];
    vec4 c2 = gl_TessCoord.z * tc_color[2];
    te_color = c0 + c1 + c2;
}

