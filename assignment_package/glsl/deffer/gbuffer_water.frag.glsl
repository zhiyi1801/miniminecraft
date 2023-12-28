#version 460 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
//in vec3 skyCol;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D noiseTexture128;
uniform sampler2D depthNoWater;
uniform sampler2D waterNormalTexture;

uniform mat4 u_View;
uniform mat4 u_ViewInv;
uniform mat4 u_Proj;
uniform mat4 u_ProjInv;

uniform float u_Near;
uniform float u_Far;
uniform float u_Time;

uniform vec3 u_LightVec;

mat3 waterTBN = mat3(vec3(1,0,0), vec3(0,0,-1), vec3(0,1,0));

vec3 UnpackNormal(vec3 packedNormal) {
    return normalize(packedNormal * 2.0 - 1.0);
}

float random (vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

vec2 random2(vec2 st)
{
    return vec2(
                fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123),
                fract(sin(dot(st.xy, vec2(87.9898,129.233))) * 321.5453123)
                );
}

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // 回到NDC
    return (2.0 * u_Near * u_Far) / (u_Far + u_Near - z * (u_Far - u_Near));
}

void main()
{
    vec3 fragPosInViewSpace = vec3(u_View * vec4(FragPos.xyz, 1.f));
    // depth with water
    float linearDepth0 = -fragPosInViewSpace.z;
    vec2 screenTex = gl_FragCoord.xy / vec2(textureSize(depthNoWater, 0));

    // coordinate without water
    float depth1 = texture2D(depthNoWater, screenTex.xy).r;
    float linearDepth1 = LinearizeDepth(depth1);

    vec4 waveParams = vec4(15, 9, -7, 22) / 20.f;
    float scale = 16;
    vec2 scaleTexCoords = FragPos.xz / scale;
    scaleTexCoords = fract(scaleTexCoords);
    scaleTexCoords += TexCoords.xy / scale;

    vec2 panner1 = u_Time * waveParams.xy + scaleTexCoords.xy;
    vec2 panner2 = u_Time * waveParams.zw + scaleTexCoords.xy;

    // normal blend
    vec3 normal1 = UnpackNormal(texture(waterNormalTexture, panner1).rgb);
    vec3 normal2 = UnpackNormal(texture(waterNormalTexture, panner2).rgb);
    vec3 worldNormal = mix(normal1, normal2, 0.5); // mix the normal

    float normalScale = 1.f;
    worldNormal = mix(vec3(0, 0, 1), worldNormal, normalScale);
    worldNormal = waterTBN * worldNormal;

    // Store the fragment position vector in the first gbuffer texture
    gPosition.xyz = FragPos;
    gPosition.a = 1;
    // Also store the per-fragment normals into the gbuffer
    gNormal.xyz = normalize(worldNormal);
    gNormal.a = 1.f;
    // And the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(diffuseTexture, TexCoords).rgb;
    vec3 col = vec3(0.2, 0.4, 0.8);
    gAlbedoSpec.rgb = col;
    //gAlbedoSpec.rgb = mix(skyCol*0.5, finalCol, factor);
    // Store specular intensity in gAlbedoSpec's alpha component
    float visibleLength = 6.f;
    gAlbedoSpec.a = min(visibleLength, abs(linearDepth1 - linearDepth0))/visibleLength;
}
