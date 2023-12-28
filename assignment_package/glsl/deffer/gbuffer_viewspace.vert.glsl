#version 330 core
in vec4 vs_Pos;             // The array of vertex positions passed to the shader
in vec4 vs_Nor;             // The array of vertex normals passed to the shader
in vec4 vs_Col;             // The array of vertex colors passed to the shader.

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 u_View;
uniform mat4 u_Proj;
uniform mat4 u_Model;
uniform mat4 u_ModelInvTr;

void main()
{
    vec4 viewPos = u_View * u_Model * vs_Pos;
    FragPos = viewPos.xyz;

    TexCoords = vs_Col.xy;

    Normal = vec3(transpose(inverse(u_View * u_Model)) * vs_Nor);

    gl_Position = u_Proj * viewPos;
}
