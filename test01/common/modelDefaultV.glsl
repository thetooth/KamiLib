#version 130
#extension GL_EXT_gpu_shader4 : enable

uniform float time;

out vec3 vertex_light_position;
out vec3 vertex_normal;

void main(void)
{
	vertex_normal = normalize(gl_NormalMatrix * gl_Normal);
	vertex_light_position = normalize(gl_LightSource[0].position.xyz);

	gl_FrontColor = gl_Color;
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
}