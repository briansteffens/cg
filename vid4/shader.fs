#version 120

varying vec2 texcoord0;
varying vec3 normal0;

uniform sampler2D sampler0;
uniform sampler2D sampler1;

uniform vec4 tint;

void main(void)
{
	vec4 sampled = texture2D(sampler0, texcoord0);

	vec3 clr = mix(sampled.rgb, tint.rgb, tint.a);
	
	gl_FragColor = vec4(clr.r, clr.g, clr.b, sampled.a);
}
