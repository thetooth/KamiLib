#version 130
#extension GL_EXT_gpu_shader4 : enable

in vec3 vertex_light_position;
in vec3 vertex_normal;

uniform sampler2D sceneTex;
uniform sampler2D sceneDepthTex;

vec3 crosshatch(){
	float val = 0.0;
	if(int(gl_FragCoord.y)%2 == 0 && int(gl_FragCoord.x)%2 == 0){
		val = 1.0;
	}else if(int(gl_FragCoord.y)%2 != 0 && int(gl_FragCoord.x)%2 != 0){
		val = 1.0;
	}
	return vec3(0.0, val/1.4, val/1.2);
}

void main()
{
	vec2 uv = gl_TexCoord[0].xy;

	float diffuse_value = max(dot(vertex_normal, vertex_light_position), 0.0);
	float fDepth = 1.0 - (gl_FragCoord.z / gl_FragCoord.w) / 100.0;
	gl_FragColor = vec4(texture2D(sceneTex, uv).rgb*diffuse_value, texture2D(sceneTex, uv).a);
}