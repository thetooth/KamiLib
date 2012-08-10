#include "common/utility.frag"

out vec4 FragmentColor;

#define KERNEL_SIZE  4.0

uniform int preset = 0;
uniform vec2 center = vec2(0.5, 0.5);

uniform float BLUR_CLAMP = 0.5/KERNEL_SIZE;
uniform float BLUR_BIAS = 0.05/KERNEL_SIZE;
uniform float BRIGHT_PASS_THRESHOLD = 0.75;
uniform float BRIGHT_PASS_OFFSET = 1.5;
float contrast = 1.0;

vec4 grade(vec4 tcolor){
	tcolor.r =  tcolor.r*1.03+0.01;
	tcolor.g = tcolor.g*1.02;
	tcolor.b = tcolor.b*0.75+0.15;
	return tcolor;
}

float vignette(float bounds, float offset, vec2 pos){
	return smoothstep(bounds, offset, distance(coord, pos));
}

vec3 rgbgrain(vec2 seed, float division, float offset){
	return vec3(
					(rand(seed)/division)+offset,
					(rand(seed*rand(seed))/division)+offset,
					(rand(seed*rand(seed*rand(seed)))/division)+offset
	);
}

void main(){
	vec2 uv = coord;
	vec3 stage0;
	switch(preset){
	case 0: // Simple
		FragmentColor = color;
		break;
	case 1: // Normal
		stage0 = grade(color).rgb;
		FragmentColor = vec4(stage0, 1.0);
		break;
	case 2: // High
		float grain = (rand(gl_FragCoord.xy*time)/20.0)+1.0;
		stage0 = (grade(color).rgb)*grain;
		FragmentColor = vec4(stage0*vignette(1.8, 0.0, vec2(0.5, 0.5)), 1.0);
		break;
	case 3: // Full Retard
		stage0 = vec3(0.0);
		stage0 += grade(color).rgb;
		stage0 *= pow( 32.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y), 0.125 );
		stage0 *= rgbgrain(gl_FragCoord.xy*time, 20.5, 1.0);
		FragmentColor = vec4(stage0, 1.0);
		break;
	default: // Debug/Experimental
		FragmentColor = vec4(color.a, color.a, color.a, 1.0);
	}
}
