
#version 450


//#define G 1.887130407e-7
#define G 1.142602313e-4
#define DELTA_T 1
#define SOFTEN 1


#define LOCAL_WORKGROUP_SIZE %v
#define NUM_SPHERES %v


layout(local_size_x=LOCAL_WORKGROUP_SIZE) in;


struct Orb {
	vec4 location;
	vec4 velocity;
};


layout(std430, binding=0) buffer Orbs0 {
	Orb orbs0[];
};

layout(std430, binding=1) buffer Orbs1 {
	Orb orbs1[];
};


void main() {
	uint gid = gl_GlobalInvocationID.x;

	if( gid < NUM_SPHERES ) {
		Orb orb = orbs0[gid];

		vec3 sum = vec3(0, 0, 0);
		for( int i = 0; i < NUM_SPHERES; i++ ) {
			vec3 dv = orbs0[i].location.xyz - orb.location.xyz;
			float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
			float divisor = sqrt(brackets * brackets * brackets);
			sum += (orbs0[i].location.w / divisor) * dv;
		}
		vec3 acceleration = sum * G;

		orb.location.xyz += orb.velocity.xyz * DELTA_T + ((DELTA_T * DELTA_T) / 2) * acceleration;
		orb.velocity.xyz += acceleration * DELTA_T;

		orbs1[gid] = orb;
	}
}


