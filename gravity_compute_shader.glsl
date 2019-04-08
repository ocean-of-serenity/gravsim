
#version 450


layout(local_size_x=128) in;


uniform uint active_buffers;

uniform uint num_spheres;


layout(std430, binding=0) readonly buffer Masses {
	float masses[];
};

layout(std430, binding=1) buffer Velocities1 {
	vec4 velocities1[];
};

layout(std430, binding=2) buffer Locations1 {
	vec4 locations1[];
};

layout(std430, binding=3) buffer Velocities2 {
	vec4 velocities2[];
};

layout(std430, binding=4) buffer Locations2 {
	vec4 locations2[];
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
				vec3 dv = locations1[i].xyz - locations1[id].xyz;
				float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
				float divisor = sqrt(brackets * brackets * brackets);
				sum += (masses[i] / divisor) * dv;
			}
			vec3 acceleration = sum * G;
			locations2[id].xyz = locations1[id].xyz + velocities1[id].xyz * DELTA_T + ((DELTA_T * DELTA_T) / 2) * acceleration;
			velocities2[id].xyz = velocities1[id].xyz + (acceleration * DELTA_T);
		}
		else {
			vec3 sum = vec3(0, 0, 0);
			for( int i = 0; i < num_spheres; i++ ) {
				vec3 dv = locations2[i].xyz - locations2[id].xyz;
				float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
				float divisor = sqrt(brackets * brackets * brackets);
				sum += (masses[i] / divisor) * dv;
			}
			vec3 acceleration = sum * G;
			locations1[id].xyz = locations2[id].xyz + velocities2[id].xyz * DELTA_T + ((DELTA_T * DELTA_T) / 2) * acceleration;
			velocities1[id].xyz = velocities2[id].xyz + (acceleration * DELTA_T);
		}
	}
}


