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

layout (location = 0) in vec4 position;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec2 outTexCoord;

void main()
{
    outTexCoord = inTexCoord;
    mat4 vpc = sharedUbo.clip * sharedUbo.proj * sharedUbo.view;
    gl_Position = vpc * dynamicUbo.model * position;
}
