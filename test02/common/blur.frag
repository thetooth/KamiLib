#version 330 core

#define KERNEL_SIZE 4.0

uniform float time							= 234587.235845;
uniform vec2  resolution					= vec2(1024.0f, 1024.0f);
uniform vec4  region						= vec4(0.0f, 0.0f, 0.0f, 0.0f);
uniform float BLUR_CLAMP					= 0.5/KERNEL_SIZE;
uniform float BLUR_BIAS						= 0.01/KERNEL_SIZE;
uniform float BRIGHT_PASS_THRESHOLD			= 0.25;
uniform float BRIGHT_PASS_OFFSET			= 0.25;
uniform float BRIGHT_PASS_COMPENSATION		= 2.01;

out vec4 FragmentColor;
uniform sampler2D sceneTex;
vec2 texcoord = vec2(gl_FragCoord.x/resolution.x, gl_FragCoord.y/resolution.y);
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
        }
else{
            color = texcolor;
        }

	}
	return color;
}


void main()
{
    vec2 blur = vec2(clamp( BLUR_BIAS, -BLUR_CLAMP, BLUR_CLAMP ));
    vec4 blurTex = texture2D(sceneTex, texcoord);
    if(region.z+region.w == 0.0f || (gl_FragCoord.x >= region.x && gl_FragCoord.y >= region.y && gl_FragCoord.x <= region.x+region.z && gl_FragCoord.y <= region.y+region.w)){
        for (float x = -KERNEL_SIZE + 1.0; x < KERNEL_SIZE; x += 1.0 ){
            for (float y = -KERNEL_SIZE + 1.0; y < KERNEL_SIZE; y += 1.0 ){
                blurTex += bright(texcoord + vec2( blur.x * x * (rand(texcoord*time)/1.0+1.0), blur.y * y * (rand(texcoord*time)/1.0+1.0)), false);
            }
		}
		blurTex /= ((KERNEL_SIZE*BRIGHT_PASS_COMPENSATION)-1.0)*((KERNEL_SIZE*BRIGHT_PASS_COMPENSATION)-1.0);
    }

	FragmentColor = vec4(blurTex.rgb, 1.0);
}
