
#version 450


layout(local_size_x=128) in;


uniform uint active_buffers;

uniform uint num_spheres;


struct Orb {
	vec4 location1;
	vec4 velocity1;
	vec4 location2;
	vec4 velocity2;
};


layout(std430, binding=0) buffer Orbs {
	Orb orbs[];
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
				vec3 dv = orbs[i].location1.xyz - orbs[id].location1.xyz;
				float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
				float divisor = sqrt(brackets * brackets * brackets);
				sum += (orbs[i].location1.w / divisor) * dv;
			}
			vec3 acceleration = sum * G;

			orbs[id].location2.xyz = orbs[id].location1.xyz + orbs[id].velocity1.xyz * DELTA_T + ((DELTA_T * DELTA_T) / 2) * acceleration;

			orbs[id].velocity2.xyz = orbs[id].velocity1.xyz + (acceleration * DELTA_T);
		}
		else {
			vec3 sum = vec3(0, 0, 0);
			for( int i = 0; i < num_spheres; i++ ) {
				vec3 dv = orbs[i].location2.xyz - orbs[id].location2.xyz;
				float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
				float divisor = sqrt(brackets * brackets * brackets);
				sum += (orbs[i].location2.w / divisor) * dv;
			}
			vec3 acceleration = sum * G;

			orbs[id].location1.xyz = orbs[id].location2.xyz + orbs[id].velocity2.xyz * DELTA_T + ((DELTA_T * DELTA_T) / 2) * acceleration;

			orbs[id].velocity1.xyz = orbs[id].velocity2.xyz + (acceleration * DELTA_T);
		}
	}
}


