#version 130
#extension GL_EXT_gpu_shader4 : enable

uniform float time = 1.0;
uniform int preset = 3;
uniform vec2 center = vec2(0.5, 0.5);
uniform sampler2D sceneTex;
uniform sampler2D depthTex;

vec2 texcoord = vec2(gl_TexCoord[0]).st;
vec4 texcolor = texture2D(sceneTex, gl_TexCoord[0].st);

float rand(vec2 co){
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main(){
	switch(preset){
	default:
		gl_FragColor = texcolor;
	}
}

