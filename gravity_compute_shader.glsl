
#version 450


layout(local_size_x=128) in;


uniform uint active_buffers;

uniform uint num_spheres;


struct Orb {
	vec4 location;
	vec4 velocity;
};


layout(std430, binding=0) buffer Orbs1 {
	Orb orbs1[];
};

layout(std430, binding=1) buffer Orbs2 {
	Orb orbs2[];
};


//#define G 1.887130407e-7
#define G 1.142602313e-4
#define DELTA_T 1
#define SOFTEN 1


void main() {
	uint id = gl_GlobalInvocationID.x;

	if( id < num_spheres ) {
		if( active_buffers == 1 ) {
			vec3 sum = vec3(0, 0, 0);
			for( int i = 0; i < num_spheres; i++ ) {
				vec3 dv = orbs1[i].location.xyz - orbs1[id].location.xyz;
				float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
				float divisor = sqrt(brackets * brackets * brackets);
				sum += (orbs1[i].location.w / divisor) * dv;
			}
			vec3 acceleration = sum * G;

			orbs2[id].location.xyz = orbs1[id].location.xyz + orbs1[id].velocity.xyz * DELTA_T + ((DELTA_T * DELTA_T) / 2) * acceleration;

			orbs2[id].velocity.xyz = orbs1[id].velocity.xyz + (acceleration * DELTA_T);
		}
		else {
			vec3 sum = vec3(0, 0, 0);
			for( int i = 0; i < num_spheres; i++ ) {
				vec3 dv = orbs2[i].location.xyz - orbs2[id].location.xyz;
				float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
				float divisor = sqrt(brackets * brackets * brackets);
				sum += (orbs2[i].location.w / divisor) * dv;
			}
			vec3 acceleration = sum * G;

			orbs1[id].location.xyz = orbs2[id].location.xyz + orbs2[id].velocity.xyz * DELTA_T + ((DELTA_T * DELTA_T) / 2) * acceleration;

			orbs1[id].velocity.xyz = orbs2[id].velocity.xyz + (acceleration * DELTA_T);
		}
	}
}


