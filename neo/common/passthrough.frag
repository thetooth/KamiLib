#version 330 core

uniform vec2 BUFFER_EXTENSITY = vec2(1024.0f, 1024.0f);
uniform sampler2D image;

out vec4 FragmentColor;

void main(void)
{
	FragmentColor = texture2D( image, vec2(gl_FragCoord.x/BUFFER_EXTENSITY.x, gl_FragCoord.y/BUFFER_EXTENSITY.y) );
}
