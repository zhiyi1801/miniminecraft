#version 330 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out float gDepthWithoutWater;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

uniform float u_Near;
uniform float u_Far;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // 回到NDC
    return (2.0 * u_Near * u_Far) / (u_Far + u_Near - z * (u_Far - u_Near));
}

void main()
{
    // Store the fragment position vector in the first gbuffer texture
    gPosition.xyz = FragPos;
    gPosition.a = LinearizeDepth(gl_FragCoord.z);
    // Also store the per-fragment normals into the gbuffer
    gNormal.xyz = normalize(Normal);
    gNormal.a = 0;
    // And the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(diffuseTexture, TexCoords).rgb;
    // Store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = texture(specularTexture, TexCoords).r;

    gDepthWithoutWater = gl_FragCoord.z;
}
