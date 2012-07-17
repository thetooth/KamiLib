#version 130

out vec4 FragmentColor;

#define KERNEL_SIZE  4.0

uniform float time;
uniform int preset = 0;
uniform vec2 center = vec2(0.5, 0.5);
uniform vec2 BUFFER_EXTENSITY = vec2(1024.0f, 1024.0f);
uniform sampler2D sceneTex;
uniform sampler2D depthTex;

uniform float BLUR_CLAMP = 0.5/KERNEL_SIZE;
uniform float BLUR_BIAS = 0.01/KERNEL_SIZE;
uniform float BRIGHT_PASS_THRESHOLD = 0.75;
uniform float BRIGHT_PASS_OFFSET = 1.5;
float contrast = 1.0;

vec2 texcoord = vec2(gl_FragCoord.x/BUFFER_EXTENSITY.x, gl_FragCoord.y/BUFFER_EXTENSITY.y);
vec4 texcolor = texture2D(sceneTex, texcoord);

float rand(vec2 co){
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec4 bright(vec2 coo, bool lowpass)
{
	vec4 color = texture2D(sceneTex, coo);
	if(lowpass)
	{
		color = max(color - BRIGHT_PASS_THRESHOLD, 0.0) / (color + BRIGHT_PASS_OFFSET);
	}

	return color;
}

vec4 gaussianblur(bool procedural)
{
	vec2 blur = vec2(clamp( BLUR_BIAS, -BLUR_CLAMP, BLUR_CLAMP ));
	vec4 blurTex = texture2D(sceneTex, texcoord);
	for (float x = -KERNEL_SIZE + 1.0; x < KERNEL_SIZE; x += 1.0 )
	{
		for (float y = -KERNEL_SIZE + 1.0; y < KERNEL_SIZE; y += 1.0 )
		{
			blurTex += bright( texcoord + vec2( blur.x * x * (rand(gl_FragCoord.xy*time)/1.0+1.0), blur.y * y * (rand(gl_FragCoord.xy*time)/1.0+1.0)), procedural);
		}

	}
	blurTex /= ((KERNEL_SIZE*2.0)-1.0)*((KERNEL_SIZE*2.0)-1.0);
	return blurTex;
}

vec4 gradient(vec4 coo)
{
	vec4 stripes = coo;
	stripes.r =  stripes.r*1.03+0.01;
	stripes.g = stripes.g*1.02;
	stripes.b = stripes.b*0.7+0.15;
	stripes.a = texcolor.a;
	return stripes;
}

float vignette(float bounds, float offset, vec2 pos)
{
	return smoothstep(bounds, offset, distance(texcoord, pos));
}

vec3 colormerge(){
	float r = (texcolor.r+texcolor.g+texcolor.b)/3.0;
	return vec3(r, r, r);
}

vec3 scanline(vec3 color){
	if(int(gl_FragCoord.y)%2 == 1){
		return color/1.05;
	}else{
		return color;
	}
}

vec3 rgbgrain(vec2 seed, float division, float offset){
	return vec3(
					(rand(seed)/division)+offset,
					(rand(seed*rand(seed))/division)+offset,
					(rand(seed*rand(seed*rand(seed)))/division)+offset
	);
}

void main(){
	vec2 uv = texcoord;
	vec3 stage0;
	switch(preset){
	case 0: // Simple
		FragmentColor = texcolor;
		break;
	case 1: // Normal
		stage0 = gradient(texcolor).rgb;
		FragmentColor = vec4(stage0, 1.0);
		break;
	case 2: // High
		float grain = (rand(gl_FragCoord.xy*time)/20.0)+1.0;
		stage0 = (gradient(texcolor).rgb)*grain;
		FragmentColor = vec4(stage0*vignette(1.8, 0.0, vec2(0.5, 0.5)), 1.0);
		break;
	case 3: // Full Retard
		stage0 = vec3(0.0);
		stage0 += gradient(texcolor).rgb;
		stage0 += gaussianblur(true).rgb;
		stage0 *= pow( 32.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y), 0.15 );
		stage0 *= rgbgrain(gl_FragCoord.xy*time, 20.5, 1.0);
		FragmentColor = vec4(stage0, 1.0);
		break;
	default: // Debug/Experimental
		FragmentColor = vec4(texcolor.a, texcolor.a, texcolor.a, 1.0);
	}
}
