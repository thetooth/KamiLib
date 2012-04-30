#version 130
#extension GL_EXT_gpu_shader4 : enable

uniform float time;
out vec2 shaderClock;

void main(void){
	shaderClock = vec2(time, time/1000.0f);
    gl_FrontColor = gl_Color;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_MultiTexCoord1;
}