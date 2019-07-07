
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


layout(std430, binding=0) readonly buffer Orbs0 {
	Orb orbs0[];
};

layout(std430, binding=1) buffer Orbs1 {
	Orb orbs1[];
};


void main() {
	if( gl_GlobalInvocationID.x >= NUM_SPHERES ) {
		return;
	}

	Orb orb = orbs0[gl_GlobalInvocationID.x];

	vec3 sum = vec3(0, 0, 0);
	for( int i = 0; i < NUM_SPHERES; i++ ) {
		const vec3 dv = orbs0[i].location.xyz - orb.location.xyz;
		const float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
		const float divisor = sqrt(brackets * brackets * brackets);
		sum += (orbs0[i].location.w / divisor) * dv;
	}
	const vec3 acceleration = sum * G;

	orb.location.xyz += DELTA_T * orb.velocity.xyz + DELTA_T * DELTA_T * 0.5 * acceleration;
	orb.velocity.xyz += DELTA_T * acceleration;

	orbs1[gl_GlobalInvocationID.x] = orb;
}


