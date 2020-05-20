#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
// some types require multiple slots

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 fragNormal;

//vec3 lightPos = vec3(-2.0,2.0,2.0);

void main() {
    mat4 mvp = ubo.proj * ubo.view * ubo.model;
    gl_Position = mvp * vec4(inPosition, 1.0);
    
    //fragNormal = inNormal; 
    fragNormal = vec3(ubo.model * vec4(inNormal, 0.0));
    fragPos = vec3(ubo.model * vec4(inPosition, 1.0));
    fragColor = inColor;
}
