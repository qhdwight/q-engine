#version 450

#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNorm;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inJoint0;
layout (location = 5) in vec4 inWeight0;

layout (binding = 0) uniform SharedUbo
{
    mat4 view;
    mat4 proj;
    mat4 clip;
    vec3 camPos;
} sharedUbo;

layout (binding = 1) uniform DynamicUbo
{
    mat4 model;
//    mat4 matrix;
//    mat4 jointMatrix[MAX_NUM_JOINTS];
//    float jointCount;
} dynamicUbo;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNorm;
layout (location = 2) out vec2 outUV0;
layout (location = 3) out vec2 outUV1;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    vec4 locPos = dynamicUbo.model * vec4(inPos, 1.0);
    outNorm = normalize(transpose(inverse(mat3(dynamicUbo.model))) * inNorm);
    outUV0 = inUV0;
    outUV1 = inUV1;
    mat4 vpc = sharedUbo.clip * sharedUbo.proj * sharedUbo.view;
    gl_Position =  vpc * locPos;
}
