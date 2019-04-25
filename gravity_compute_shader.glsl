
#version 450


#define LOCAL_WORKGROUP_SIZE	256

#define NUM_ORBS				32768
#define SHARED_MEMORY_SIZE		40960

#define ORB_SIZE				32

#define NUM_SHARED_ORBS			SHARED_MEMORY_SIZE / ORB_SIZE
#define ORBS_PER_INVOCATION		NUM_SHARED_ORBS / LOCAL_WORKGROUP_SIZE
#define FEWER_ORBS_LAST_PASS	NUM_ORBS % NUM_SHARED_ORBS != 0
#define NUM_PASSES				NUM_ORBS / NUM_SHARED_ORBS + (FEWER_ORBS_LAST_PASS ? 1 : 0)
#define NUM_ORBS_LAST_PASS		NUM_ORBS - (NUM_SHARED_ORBS * (NUM_PASSES - 1))

//#define G 1.887130407e-7
#define G 1.142602313e-4
#define DELTA_T 1
#define SOFTEN 1


layout(local_size_x=LOCAL_WORKGROUP_SIZE) in;


uniform uint active_buffers;


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


shared Orb shared_orbs[NUM_SHARED_ORBS];


void main() {
	uint gid = gl_GlobalInvocationID.x;
	uint lid = gl_LocalInvocationID.x;

	if( gid < NUM_ORBS ) {
		if( active_buffers == 1 ) {
			Orb orb = orbs1[gid];

			vec3 sum = vec3(0, 0, 0);
			for( int p = 0; p < NUM_PASSES; p++ ) {
				uint gi = p * NUM_SHARED_ORBS + lid;
				uint li = lid;

#if FEWER_ORBS_LAST_PASS
				uint pass_num_orbs = (p == NUM_PASSES - 1 ? NUM_ORBS_LAST_PASS : ORBS_PER_INVOCATION);
#else
				uint pass_num_orbs = ORBS_PER_INVOCATION;
#endif

				for( int i = 0; i < pass_num_orbs; i++, gi += ORBS_PER_INVOCATION, li += ORBS_PER_INVOCATION ) {
					shared_orbs[li] = orbs1[gi];
				}
	
				memoryBarrierShared();
				barrier();

				for( int i = 0; i < pass_num_orbs; i++ ) {
					vec3 dv = shared_orbs[i].location.xyz - orb.location.xyz;
					float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
					float divisor = sqrt(brackets * brackets * brackets);
					sum += (shared_orbs[i].location.w / divisor) * dv;
				}
			}
			vec3 acceleration = sum * G;
	
			orb.location.xyz += orb.velocity.xyz * DELTA_T + ((DELTA_T * DELTA_T) / 2) * acceleration;
			orb.velocity.xyz += acceleration * DELTA_T;

			orbs2[gid] = orb;
		}
		else {
			Orb orb = orbs2[gid];

			vec3 sum = vec3(0, 0, 0);
			for( int p = 0; p < NUM_PASSES; p++ ) {
				uint gi = p * NUM_SHARED_ORBS + lid;
				uint li = lid;

#if FEWER_ORBS_LAST_PASS
				uint pass_num_orbs = (p == NUM_PASSES - 1 ? NUM_ORBS_LAST_PASS : ORBS_PER_INVOCATION);
#else
				uint pass_num_orbs = ORBS_PER_INVOCATION;
#endif

				for( int i = 0; i < pass_num_orbs; i++, gi += ORBS_PER_INVOCATION, li += ORBS_PER_INVOCATION ) {
					shared_orbs[li] = orbs2[gi];
				}
	
				memoryBarrierShared();
				barrier();
	
				for( int i = 0; i < pass_num_orbs; i++ ) {
					vec3 dv = shared_orbs[i].location.xyz - orb.location.xyz;
					float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
					float divisor = sqrt(brackets * brackets * brackets);
					sum += (shared_orbs[i].location.w / divisor) * dv;
				}
			}
			vec3 acceleration = sum * G;
	
			orb.location.xyz += orb.velocity.xyz * DELTA_T + ((DELTA_T * DELTA_T) / 2) * acceleration;
			orb.velocity.xyz += acceleration * DELTA_T;

			orbs1[gid] = orb;
		}
	}
}


