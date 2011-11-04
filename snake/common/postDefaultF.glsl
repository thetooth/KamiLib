#version 120

uniform float pixel_w = 4;
uniform float pixel_h = 4;
uniform sampler2D sceneTex;

void main()
{
	vec2 uv = gl_TexCoord[0].xy;
	vec3 tc = texture2D(sceneTex, uv).rgb;

	float dx = pixel_w*(1.0/800);
	float dy = pixel_h*(1.0/480);
	vec2 coord = vec2(dx*floor(uv.x/dx), dy*floor(uv.y/dy));
	//tc = texture2D(sceneTex, coord).rgb;
	
	float dist = distance(uv, vec2(0.5,0.5));
	tc *= smoothstep(1.95, 0.0, dist);
	gl_FragColor = vec4(tc, 1.0);
}

