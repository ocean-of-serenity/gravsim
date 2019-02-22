
#version 450


layout(local_size_x=256) in;


uniform float speedMultiplier;
uniform uint numSpheres;

layout(std430, binding=0) buffer Locations {
    vec4 locations[];
};

layout(std430, binding=1) buffer Masses {
    float masses[];
};

layout(std430, binding=2) buffer Velocities {
    vec4 velocities[];
};

layout(std430, binding=3) buffer Models {
    mat4 models[];
};


#define G 1.887130407e-7
#define deltaT 1
#define softeningFactor 1


void main() {
    uint id = gl_GlobalInvocationID.x;

    if( id < numSpheres ) {
/*		vec3 sum = vec3(0, 0, 0);
		for(int i = 1; i < numSpheres; i++) {
			vec3 dv = locations[i].xyz - locations[id].xyz;
			float brackets = dot(dv, dv) + softeningFactor * softeningFactor;
			float divisor = sqrt(brackets * brackets * brackets);
			sum += (masses[i] * divisor) * dv;
		}
		vec3 acceleration = sum * (G * masses[id]);
		vec3 velocity = velocities[id].xyz + (acceleration * deltaT);
		vec3 location = locations[id].xyz + ((0.5 * deltaT) * (velocities[id].xyz + velocity));
*/
		barrier();

		vec3 location = vec3(0, 0, 0);
		locations[id].xyz = location;
        models[id][3].xyz = location;
    }
	else {
		barrier();
	}
}


