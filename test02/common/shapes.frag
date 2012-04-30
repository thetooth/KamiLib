#define M_PI 3.14159265358979323846

uniform sampler2D tex0;
uniform float time;
uniform vec2 resolution;
uniform vec2 mouse;
const mat2 rotation = mat2( cos(M_PI/4.0), sin(M_PI/4.0),
                           -sin(M_PI/4.0), cos(M_PI/4.0));

vec4 circle(vec4 fragColor, vec2 pos, float radius, vec4 color){
	return mix(color, fragColor, smoothstep(380.25, 420.25, dot(pos/radius, pos)));
}

void main(void){
	vec2 pos = mod(gl_FragCoord.xy, vec2(resolution));
	vec4 color = texture2D(tex0, gl_TexCoord[0].xy);
    gl_FragColor = circle(color, pos + vec2(mouse), 16, vec4(1.0, 1.0, 1.0, 1.0));
}