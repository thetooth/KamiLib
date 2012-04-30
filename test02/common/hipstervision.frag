#version 130
#extension GL_EXT_gpu_shader4 : enable

#define KERNEL_SIZE  4.0

uniform float time;
uniform int preset = 3;
uniform vec2 center = vec2(0.5, 0.5);
uniform sampler2D sceneTex;
uniform sampler2D depthTex;

uniform float BLUR_CLAMP = 0.5/KERNEL_SIZE;
uniform float BLUR_BIAS = 0.1/KERNEL_SIZE;
uniform float BRIGHT_PASS_THRESHOLD = 0.75;
uniform float BRIGHT_PASS_OFFSET = 1.5;
float contrast = 1.0;

vec2 texcoord = vec2(gl_TexCoord[0]).st;
vec4 texcolor = texture2D(sceneTex, gl_TexCoord[0].st);

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

vec4 dreamvision(){
	vec2 uv = gl_TexCoord[0].xy;
  vec4 c = texture2D(sceneTex, uv);

  c += texture2D(sceneTex, uv+0.001);
  c += texture2D(sceneTex, uv+0.003);
  c += texture2D(sceneTex, uv+0.005);
  c += texture2D(sceneTex, uv+0.007);
  c += texture2D(sceneTex, uv+0.009);
  c += texture2D(sceneTex, uv+0.011);

  c += texture2D(sceneTex, uv-0.001);
  c += texture2D(sceneTex, uv-0.003);
  c += texture2D(sceneTex, uv-0.005);
  c += texture2D(sceneTex, uv-0.007);
  c += texture2D(sceneTex, uv-0.009);
  c += texture2D(sceneTex, uv-0.011);

  //c.rgb = vec3((c.r+c.g+c.b)/3.0);
  c = c / 9.5;
  return c;
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

void main(){
	float grain;
	vec4 precolor;
	vec3 stage0;
	switch(preset){
	case 0: // Simple
		gl_FragColor = texcolor;
		break;
	case 1: // Normal
		stage0 = gradient(texcolor).rgb;
		gl_FragColor = vec4(stage0, 1.0);
		break;
	case 2: // High
		grain = (rand(gl_FragCoord.xy*time)/20.0)+1.0;
		stage0 = (gradient(texcolor).rgb)*grain;
		gl_FragColor = vec4(stage0*vignette(1.8, 0.0, vec2(0.5, 0.5)), 1.0);
		break;
	case 3: // Full Retard
		grain = (rand(gl_FragCoord.xy+texcolor.gb*time)/20.0)+1.0;
		//precolor = vec4(colormerge(), 1.0);
		precolor = texcolor;
		stage0 = (gradient(precolor).rgb)*grain+gaussianblur(true).rgb;
		gl_FragColor = vec4(stage0, 1.0);
		break;
	default:
		gl_FragColor = vec4(texcolor.a, texcolor.a, texcolor.a, 1.0);
	}
}

