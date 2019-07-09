
#version 450


//#define G 1.887130407e-7
#define G 1.142602313e-4
#define DELTA_T 1
#define SOFTEN 1


#define LOCAL_WORKGROUP_SIZE %v
#define NUM_SPHERES %v
#define NUM_TILES %v


layout(local_size_x=LOCAL_WORKGROUP_SIZE) in;


layout(std430, binding=0) buffer Locations0 {
	vec4 locations0[];
};

layout(std430, binding=1) readonly buffer Locations1 {
	vec4 locations1[];
};

layout(std430, binding=2) readonly buffer Velocities {
	vec4 velocities[];
};


shared vec4 shared_locations[LOCAL_WORKGROUP_SIZE];


void main() {
	if( gl_GlobalInvocationID.x >= NUM_SPHERES ) {
		return;
	}

	vec4 location = locations1[gl_GlobalInvocationID.x];

	vec4 prefetch_location;
	if( gl_LocalInvocationID.x < NUM_SPHERES ) {
		prefetch_location = locations1[gl_LocalInvocationID.x];
	}

	vec3 sum = vec3(0, 0, 0);
	for( int tile = 0; tile < NUM_TILES; tile++ ) {
		shared_locations[gl_LocalInvocationID.x] = prefetch_location;
		memoryBarrierShared();
		barrier();

		const uint tile_fetch_index = (tile + 1) * LOCAL_WORKGROUP_SIZE + gl_LocalInvocationID.x;
		if( tile_fetch_index < NUM_SPHERES ) {
			prefetch_location = locations1[tile_fetch_index];
		}
		barrier();

		const uint tile_start_index = tile * LOCAL_WORKGROUP_SIZE;
		for( int i = 0; tile_start_index + i < NUM_SPHERES && i < LOCAL_WORKGROUP_SIZE; i++ ) {
			const vec3 dv = shared_locations[i].xyz - location.xyz;
			const float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
			const float divisor = sqrt(brackets * brackets * brackets);
			sum += (shared_locations[i].w / divisor) * dv;
		}
	}
	const vec3 acceleration = sum * G;

	const vec4 velocity = velocities[gl_GlobalInvocationID.x];

	location.xyz += DELTA_T * velocity.xyz + DELTA_T * DELTA_T * 0.5 * acceleration;

	locations0[gl_GlobalInvocationID.x] = location;
}


