#version 450

#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord_0;
layout (location = 3) in vec2 inTexCoord_1;
layout (location = 4) in vec4 inJoint0;
layout (location = 5) in vec4 inWeight0;

layout (set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 proj;
    mat4 clip;
    vec3 pos;
} camera;

layout (set = 2, binding = 0) uniform Model
{
    mat4 transform;
//    mat4 matrix;
//    mat4 jointMatrix[MAX_NUM_JOINTS];
//    float jointCount;
} model;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNorm;
layout (location = 2) out vec2 outTexCoord_0;
layout (location = 3) out vec2 outTexCoord_1;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    vec4 locPos = model.transform * vec4(inPosition, 1.0);
    outNorm = normalize(transpose(inverse(mat3(model.transform))) * inNormal);
    outTexCoord_0 = inTexCoord_0;
    outTexCoord_1 = inTexCoord_1;
    mat4 vpc = camera.clip * camera.proj * camera.view;
    gl_Position =  vpc * locPos;
}
