#version 330

uniform sampler2D src_texture_0;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform vec4 globalColor;

in vec4  position;
in vec4  normal;
in vec4  color;
in vec2  texcoord;

flat out vec4 colorV;

void main()
{

	gl_Position = projectionMatrix * modelViewMatrix * position;
	
	vec2 screenspacePos = gl_Position.xy / gl_Position.w;
	screenspacePos *= 0.5;
	screenspacePos += vec2(0.5);
	// screenspacePos now contains the sample pos as if mapped to the normalised screen.

	vec4 texel = texture(src_texture_0, screenspacePos);

	colorV =  texel * globalColor;
}