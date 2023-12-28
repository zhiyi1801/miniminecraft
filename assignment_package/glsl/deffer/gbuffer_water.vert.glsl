#version 460 core
in vec4 vs_Pos;             // The array of vertex positions passed to the shader
in vec4 vs_Nor;             // The array of vertex normals passed to the shader
in vec4 vs_Col;             // The array of vertex colors passed to the shader.

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;
out vec3 skyCol;

uniform mat4 u_ViewProj;
uniform mat4 u_Model;
uniform mat4 u_ModelInvTr;
uniform mat4 u_View;
uniform float u_Time;

uniform sampler2D waterNormalTexture;

vec3 getBump(vec3 worldPos)
{
    worldPos.y += (sin(float(u_Time * 400 * 0.3) + worldPos.z * 2 + worldPos.x * 2) * 0.09);

    return worldPos;
}

void main()
{
    vec4 worldPos = u_Model * (vs_Pos - vec4(0.f, 0.4f, 0.f, 0.f));
    worldPos.xyz = getBump(worldPos.xyz);

    FragPos = worldPos.xyz;

    TexCoords = vs_Col.xy;

    Normal = vec3(u_ModelInvTr * vs_Nor);

    gl_Position = u_ViewProj * worldPos;
}
