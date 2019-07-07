
#version 450


//#define G 1.887130407e-7
#define G 1.142602313e-4
#define DELTA_T 1
#define SOFTEN 1


#define LOCAL_WORKGROUP_SIZE %v
#define NUM_SPHERES %v


layout(local_size_x=LOCAL_WORKGROUP_SIZE) in;


layout(std430, binding=0) readonly buffer Locations0 {
	vec4 locations0[];
};

layout(std430, binding=1) buffer Locations1 {
	vec4 locations1[];
};

layout(std430, binding=2) buffer Velocities {
	vec4 velocities[];
};


void main() {
	if( gl_GlobalInvocationID.x >= NUM_SPHERES ) {
		return;
	}

	vec4 location = locations0[gl_GlobalInvocationID.x];

	vec3 sum = vec3(0, 0, 0);
	for( int i = 0; i < NUM_SPHERES; i++ ) {
		if( i != gl_GlobalInvocationID.x ) {
			const vec3 dv = locations0[i].xyz - location.xyz;
			const float dv_mag = length(dv);
			const float divisor = dv_mag * dv_mag * dv_mag;
			sum += (locations0[i].w / divisor) * dv;
		}
	}
	const vec3 acceleration = sum * G;

	vec4 velocity = velocities[gl_GlobalInvocationID.x];

	location.xyz += DELTA_T * velocity.xyz + DELTA_T * DELTA_T * 0.5 * acceleration;
	velocity.xyz += DELTA_T * acceleration;

	locations1[gl_GlobalInvocationID.x] = location;
	velocities[gl_GlobalInvocationID.x] = velocity;
}


