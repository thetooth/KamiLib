#version 330 core

#define KERNEL_SIZE 4.0

uniform float time							= 234587.235845;
uniform float BLUR_CLAMP					= 0.5/KERNEL_SIZE;
uniform float BLUR_BIAS						= 0.01/KERNEL_SIZE;
uniform float BRIGHT_PASS_THRESHOLD			= 0.75;
uniform float BRIGHT_PASS_OFFSET			= 0.25;
uniform float BRIGHT_PASS_COMPENSATION		= 2.01;
uniform vec2  BUFFER_EXTENSITY				= vec2(1024.0f, 1024.0f);

out vec4 FragmentColor;
uniform sampler2D sceneTex;

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
		if(color.r+color.g+color.b > BRIGHT_PASS_THRESHOLD*3.0){
			color = texcolor+(max(color - BRIGHT_PASS_THRESHOLD, 0.0) / (color + BRIGHT_PASS_OFFSET));
		}else{
			color = texcolor;
		}
	}
	return color;
}

void main()
{
	vec2 blur = vec2(clamp( BLUR_BIAS, -BLUR_CLAMP, BLUR_CLAMP ));
	vec4 blurTex = texture2D(sceneTex, texcoord);
	for (float x = -KERNEL_SIZE + 1.0; x < KERNEL_SIZE; x += 1.0 )
	{
		for (float y = -KERNEL_SIZE + 1.0; y < KERNEL_SIZE; y += 1.0 )
		{
			blurTex += bright(texcoord + vec2( blur.x * x * (rand(texcoord*time)/1.0+1.0), blur.y * y * (rand(texcoord*time)/1.0+1.0)), false);
		}
	}
	blurTex /= ((KERNEL_SIZE*BRIGHT_PASS_COMPENSATION)-1.0)*((KERNEL_SIZE*BRIGHT_PASS_COMPENSATION)-1.0);
	FragmentColor = vec4(blurTex.rgb, 1.0);
}