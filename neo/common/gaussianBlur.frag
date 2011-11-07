#version 130
#extension GL_EXT_gpu_shader4 : enable

#define PROCEDURAL 0
#define KERNEL_SIZE  4.0
#define blurclamp 0.02/KERNEL_SIZE
#define bias 0.01/KERNEL_SIZE

uniform float time;
uniform int preset = 3;
uniform vec2 center = vec2(0.5, 0.5);
uniform sampler2D sceneTex;
uniform sampler2D depthTex;

const float BRIGHT_PASS_THRESHOLD = 0.75;
const float BRIGHT_PASS_OFFSET = 1.5;

vec2 texcoord = vec2(gl_TexCoord[0]).st;
vec4 texcolor = texture2D(sceneTex, gl_TexCoord[0].st);

float rand(vec2 co){
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec4 bright(vec2 coo, int lowpass)
{
	vec4 color = texture2D(sceneTex, coo);
	if(lowpass == 1)
	{
		color = max(color - BRIGHT_PASS_THRESHOLD, 0.0) / (color + BRIGHT_PASS_OFFSET);
	}

	return color;
}

void main()
{
	vec2 blur = vec2(clamp( bias, -blurclamp, blurclamp ));
	vec4 blurTex = texture2D(sceneTex, texcoord);
	for (float x = -KERNEL_SIZE + 1.0; x < KERNEL_SIZE; x += 1.0 )
	{
		for (float y = -KERNEL_SIZE + 1.0; y < KERNEL_SIZE; y += 1.0 )
		{
			blurTex += bright( texcoord + vec2( blur.x * x * (rand(gl_FragCoord.xy*time)/4.0+1.0), blur.y * y * (rand(gl_FragCoord.xy*time)/4.0+1.0)), PROCEDURAL);
		}
	}
	blurTex /= ((KERNEL_SIZE*2.0)-1.0)*((KERNEL_SIZE*2.0)-1.0);
	gl_FragColor = blurTex;
}