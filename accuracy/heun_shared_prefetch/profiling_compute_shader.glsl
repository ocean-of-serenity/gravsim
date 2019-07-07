
#version 450


//#define G 1.887130407e-7
#define G 1.142602313e-4
#define DELTA_T 1
#define SOFTEN 1


#define LOCAL_WORKGROUP_SIZE %v
#define NUM_SPHERES %v
#define NUM_TILES %v


layout(local_size_x=LOCAL_WORKGROUP_SIZE) in;


layout(std430, binding=0) readonly buffer Locations0 {
	vec4 locations0[];
};

layout(std430, binding=2) buffer Velocities {
	vec4 velocities[];
};

layout(std430, binding=3) buffer Results {
	vec4 results[];
};


shared vec4 shared_locations[LOCAL_WORKGROUP_SIZE];
shared vec4 shared_results[LOCAL_WORKGROUP_SIZE];


void main() {
	if( gl_GlobalInvocationID.x >= NUM_SPHERES ) {
		return;
	}

	vec4 location = locations0[gl_GlobalInvocationID.x];

	vec4 prefetch_location;
	if( gl_LocalInvocationID.x < NUM_SPHERES ) {
		prefetch_location = locations0[gl_LocalInvocationID.x];
	}


	float md = 0;
	vec3 sum = vec3(0, 0, 0);
	for( int tile = 0; tile < NUM_TILES; tile++ ) {
		shared_locations[gl_LocalInvocationID.x] = prefetch_location;
		memoryBarrierShared();
		barrier();

		const uint tile_fetch_index = (tile + 1) * LOCAL_WORKGROUP_SIZE + gl_LocalInvocationID.x;
		if( tile_fetch_index < NUM_SPHERES ) {
			prefetch_location = locations0[tile_fetch_index];
		}
		barrier();

		const uint tile_start_index = tile * LOCAL_WORKGROUP_SIZE;
		for( int i = 0; tile_start_index + i < NUM_SPHERES && i < LOCAL_WORKGROUP_SIZE; i++ ) {
			const vec3 dv = shared_locations[i].xyz - location.xyz;
			if( tile_start_index + i != gl_GlobalInvocationID.x ) {
				md += shared_locations[i].w / length(dv);
			}
			const float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
			const float divisor = sqrt(brackets * brackets * brackets);
			sum += (shared_locations[i].w / divisor) * dv;
		}
	}
	const float potential_energy = 0.5 * G * location.w * md;
	const vec3 acceleration = G * sum;

	vec4 old_velocity = velocities[gl_GlobalInvocationID.x];
	vec3 new_velocity = old_velocity.xyz + old_velocity * DELTA_T;
	vec3 velocity = 0.5 * (old_velocity.xyz + new_velocity);

	const float magnitude = length(velocity);
	const float kinetic_energy = 0.5 * location.w * (magnitude * magnitude);
	const float energy = kinetic_energy - potential_energy;

	const float angular_momentum = cross(location.xyz, location.w * velocity).y;

	const float gravitational_force = location.w * length(acceleration);

	shared_results[gl_LocalInvocationID.x] = vec4(energy, angular_momentum, gravitational_force, 0);
	memoryBarrierShared();
	barrier();
	for( int stride = LOCAL_WORKGROUP_SIZE >> 1; stride > 0; stride >>= 1 ) {
		if( gl_LocalInvocationID.x < stride && gl_GlobalInvocationID.x + stride < NUM_SPHERES ) {
			shared_results[gl_LocalInvocationID.x] += shared_results[gl_LocalInvocationID.x + stride];
		}
		memoryBarrierShared();
		barrier();
	}
	if( gl_LocalInvocationID.x == 0 ) {
		results[gl_WorkGroupID.x] = shared_results[0];
	}
}


