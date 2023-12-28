#version 460 core
in vec4 vs_Pos;             // The array of vertex positions passed to the shader
in vec4 vs_Nor;             // The array of vertex normals passed to the shader
in vec4 vs_Col;             // The array of vertex colors passed to the shader.

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 u_ViewProj;
uniform mat4 u_Model;

void main()
{
    vs_out.FragPos = vec3(u_Model * vs_Pos);
    vs_out.Normal = transpose(inverse(mat3(u_Model))) * vec3(vs_Nor);
    vs_out.TexCoords = vs_Col.xy;
    gl_Position = u_ViewProj * u_Model * vs_Pos;
}
