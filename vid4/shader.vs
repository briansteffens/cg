#version 120

attribute vec3 position;
attribute vec2 texcoord;
attribute vec3 normal;

varying vec2 texcoord0;
varying vec3 normal0;

uniform mat4 transform;

void main(void)
{
	gl_Position = transform * vec4(position, 1.0);
	texcoord0 = texcoord;
	normal0 = (transform * vec4(normal, 0.0)).xyz;
}
