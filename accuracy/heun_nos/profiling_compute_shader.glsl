
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


struct Result {
	vec4 momentum_energy;
	vec4 force;
};

layout(std430, binding=3) buffer Results {
	Result results[];
};


shared vec4 shared_locations[LOCAL_WORKGROUP_SIZE];
shared Result shared_results[LOCAL_WORKGROUP_SIZE];


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

	const vec4 old_velocity = velocities[gl_GlobalInvocationID.x];
	const vec4 new_velocity = vec4(old_velocity.xyz + DELTA_T * acceleration, 0);
	const vec4 velocity = 0.5 * (old_velocity + new_velocity);

	const float magnitude = length(velocity.xyz);
	const float kinetic_energy = 0.5 * location.w * (magnitude * magnitude);
	const float energy = kinetic_energy - potential_energy;

	const vec3 angular_momentum = cross(location.xyz, location.w * velocity.xyz);

	const vec3 gravitational_force = location.w * acceleration;

	shared_results[gl_LocalInvocationID.x] = Result(
			vec4(angular_momentum, energy),
			vec4(gravitational_force, 0)
	);
	memoryBarrierShared();
	barrier();
	for( int stride = LOCAL_WORKGROUP_SIZE >> 1; stride > 0; stride >>= 1 ) {
		if( gl_LocalInvocationID.x < stride && gl_GlobalInvocationID.x + stride < NUM_SPHERES ) {
			shared_results[gl_LocalInvocationID.x].momentum_energy += shared_results[gl_LocalInvocationID.x + stride].momentum_energy;
			shared_results[gl_LocalInvocationID.x].force += shared_results[gl_LocalInvocationID.x + stride].force;
		}
		memoryBarrierShared();
		barrier();
	}
	if( gl_LocalInvocationID.x == 0 ) {
		results[gl_WorkGroupID.x] = shared_results[0];
	}
}


