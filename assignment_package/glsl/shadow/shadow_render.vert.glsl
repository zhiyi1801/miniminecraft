#version 330 core
in vec4 vs_Pos;             // The array of vertex positions passed to the shader
in vec4 vs_Nor;             // The array of vertex normals passed to the shader
in vec4 vs_Col;             // The array of vertex colors passed to the shader.

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} vs_out;

uniform mat4 u_ViewProj;
uniform mat4 u_Model;
uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_ModelInvTr;

void main()
{
    vs_out.FragPos = vec3(u_ModelInvTr * vs_Pos);
    vs_out.Normal = vec3(u_ModelInvTr * vs_Nor);
    vs_out.TexCoords = vs_Col.xy;
    vs_out.FragPosLightSpace = u_LightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
    gl_Position = u_ViewProj * u_Model * vs_Pos;
}
