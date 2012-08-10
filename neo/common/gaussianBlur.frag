#include "common/utility.frag"

#define KERNEL_SIZE 4.0

uniform float BLUR_CLAMP					= 0.5/KERNEL_SIZE;
uniform float BLUR_BIAS						= 0.01/KERNEL_SIZE;
uniform float BRIGHT_PASS_THRESHOLD			= 0.75;
uniform float BRIGHT_PASS_OFFSET			= 0.25;
uniform float BRIGHT_PASS_COMPENSATION		= 2.01;

uniform sampler2D slave;
out vec4 FragmentColor;

void main()
{
	vec2 blur = vec2(clamp( BLUR_BIAS, -BLUR_CLAMP, BLUR_CLAMP ));
	vec4 blurTex = vec4(0.0);
	if(color.r+color.g+color.b > BRIGHT_PASS_THRESHOLD*3.0){
		//blurTex = color;
	}
	for (float x = -KERNEL_SIZE + 1.0; x < KERNEL_SIZE; x += 1.0 )
	{
		for (float y = -KERNEL_SIZE + 1.0; y < KERNEL_SIZE; y += 1.0 )
		{
			vec4 tcolor = texture2D(image, coord + vec2( blur.x * x * (rand(coord*time)/1.0+1.0), blur.y * y * (rand(coord*time)/1.0+1.0)));
			if(tcolor.r+tcolor.g+tcolor.b > BRIGHT_PASS_THRESHOLD*3.0){
				blurTex += tcolor;
			}
		}
	}
	blurTex /= ((KERNEL_SIZE*BRIGHT_PASS_COMPENSATION)-1.0)*((KERNEL_SIZE*BRIGHT_PASS_COMPENSATION)-1.0);
	//blurTex = (max(blurTex - BRIGHT_PASS_THRESHOLD, 0.0) / (blurTex + BRIGHT_PASS_OFFSET));
	blurTex /= 4.0;

	FragmentColor = vec4(color.rgb+blurTex.rgb, 1.0);
}