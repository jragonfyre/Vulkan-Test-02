#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

vec3 lightPos = vec3(-2.0,2.0,2.0);

void main() {
	vec3 lightDir = normalize(lightPos-fragPos);
	vec3 normal = normalize(fragNormal);
	float ratio = dot(lightDir, normal); // light ratio
	ratio = clamp(ratio, 0.1, 1.0);
	//float ratio = 1.0;
	//outColor = vec4((0.5*normal) + vec3(0.5,0.5,0.5), 1.0);
	outColor = vec4(ratio*fragColor, 1.0);
}