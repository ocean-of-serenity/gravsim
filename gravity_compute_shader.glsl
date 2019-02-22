
#version 450


layout(local_size_x=256) in;


uniform float speedMultiplier;
uniform uint numSpheres;

layout(std430, binding=0) buffer Masses {
	float masses[];
};

layout(std430, binding=1) buffer Velocities {
	vec4 velocities[];
};

layout(std430, binding=2) buffer Models {
	mat4 models[];
};


#define G 1.887130407e-7
#define DELTA_T 1e2
#define SOFTEN 1e1


void main() {
	uint id = gl_GlobalInvocationID.x;

	if( id < numSpheres ) {
		vec3 sum = vec3(0, 0, 0);
		for( int i = 0; i < numSpheres; i++ ) {
			vec3 dv = models[i][3].xyz - models[id][3].xyz;
			float brackets = dot(dv, dv) + SOFTEN * SOFTEN;
			float divisor = sqrt(brackets * brackets * brackets);
			sum += (masses[i] / divisor) * dv;
		}
		vec3 acceleration = sum * G;
		vec3 location = models[id][3].xyz + velocities[id].xyz * DELTA_T + ((DELTA_T * DELTA_T) / 2) * acceleration;
		vec3 velocity = velocities[id].xyz + (acceleration * DELTA_T);

		barrier();

		models[id][3].xyz = location;
		velocities[id].xyz = velocity;
	}
	else {
		barrier();
	}
}


