#version 400

#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform SharedUbo
{
    mat4 view;
    mat4 proj;
    mat4 clip;
} sharedUbo;

layout (binding = 1) uniform DynamicUbo
{
    mat4 model;
} dynamicUbo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main()
{
//    outColor = inColor;
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
    mat4 vpc = sharedUbo.clip * sharedUbo.proj * sharedUbo.view;
    gl_Position = vpc * dynamicUbo.model * vec4(inPosition, 1.0);
}
