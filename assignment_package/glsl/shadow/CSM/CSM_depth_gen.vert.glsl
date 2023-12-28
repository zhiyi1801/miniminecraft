#version 460 core
layout (location = 0) in vec3 vs_Pos;

uniform mat4 u_Model;

layout (std140, binding = 0) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};

void main()
{
    gl_Position = u_Model * vec4(vs_Pos, 1.0);
}
