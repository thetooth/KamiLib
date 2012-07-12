#version 130
#extension GL_EXT_gpu_shader4 : enable

uniform float time = 234587.235845;

float rand(vec2 co){
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main(void){
    gl_FrontColor = vec4(rand(gl_Color.rb), 0.0, 0.0, gl_Color.a);//gl_Color;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}