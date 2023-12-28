#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in float NightMixTerm;
in float Time; //0-24
in float HaloTerm;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gSSAO;
uniform sampler2DArray shadowMap;
uniform sampler2D depthAll;
uniform sampler2D depthNoWater;
uniform sampler2D noiseTexture128;

uniform vec3 u_LightVec;
uniform mat4 u_View;
uniform mat4 u_ProjInv;
uniform vec3 u_SunDirView;

uniform bool u_showDepth;
uniform bool u_showLayer;
uniform bool u_useSSAO;

uniform float u_FarPlane;
uniform float u_Time;

layout (std140, binding = 0) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;   // number of frusta - 1

vec3 sunColArr[24] = {
    vec3(2, 2, 0.5),          // 0-1
    vec3(2, 1.5, 0.7),        // 1 - 2
    vec3(1.5, 1, 1),          // 2 - 3
    vec3(1, 1, 1),          // 3 - 4
    vec3(1, 1, 1),          // 4 - 5
    vec3(1, 1, 1),          // 5 - 6
    vec3(1, 1, 1),          // 6 - 7
    vec3(1, 1, 1),          // 7 - 8
    vec3(1.2, 1, 0.8),          // 8 - 9
    vec3(1.5, 1, 0.7),          // 9 - 10
    vec3(2, 1.5, 0.5),          // 10 - 11
    vec3(1.5, 1.2, 0.6),          // 11 - 12
    vec3(0.8, 1, 0.7),      // 12 - 13
    vec3(0.3, 0.5, 0.9),    // 13 - 14
    vec3(0.3, 0.5, 0.9),    // 14 - 15
    vec3(0.3, 0.5, 0.9),    // 15 - 16
    vec3(0.3, 0.5, 0.9),    // 16 - 17
    vec3(0.3, 0.5, 0.9),    // 17 - 18
    vec3(0.3, 0.5, 0.9),    // 18 - 19
    vec3(0.3, 0.5, 0.9),    // 19 - 20
    vec3(0.3, 0.5, 0.9),    // 20 - 21
    vec3(0.3, 0.5, 0.9),    // 21 - 22
    vec3(0.3, 0.5, 0.9),    // 22 - 23
    vec3(0.3, 0.5, 0.9)     // 23 - 24(0)
};

vec3 skyColArray[24] = {
    vec3(0.1, 0.6, 0.9),        // 0-1
    vec3(0.1, 0.6, 0.9),        // 1 - 2
    vec3(0.1, 0.6, 0.9),        // 2 - 3
    vec3(0.1, 0.6, 0.9),        // 3 - 4
    vec3(0.1, 0.6, 0.9),        // 4 - 5
    vec3(0.1, 0.6, 0.9),        // 5 - 6
    vec3(0.1, 0.6, 0.9),        // 6 - 7
    vec3(0.1, 0.6, 0.9),        // 7 - 8
    vec3(0.1, 0.6, 0.9),        // 8 - 9
    vec3(0.1, 0.6, 0.9),        // 9 - 1
    vec3(0.1, 0.6, 0.9),        // 10 - 11
    vec3(0.1, 0.6, 0.9),        // 11 - 12
    vec3(0.1, 0.6, 0.9),        // 12 - 13
    vec3(0.02, 0.2, 0.27),      // 13 - 14
    vec3(0.02, 0.2, 0.27),      // 14 - 15
    vec3(0.02, 0.2, 0.27),      // 15 - 16
    vec3(0.02, 0.2, 0.27),      // 16 - 17
    vec3(0.02, 0.2, 0.27),      // 17 - 18
    vec3(0.02, 0.2, 0.27),      // 18 - 19
    vec3(0.02, 0.2, 0.27),      // 19 - 20
    vec3(0.02, 0.2, 0.27),      // 20 - 21
    vec3(0.02, 0.2, 0.27),      // 21 - 22
    vec3(0.02, 0.2, 0.27),      // 22 - 23
    vec3(0.02, 0.2, 0.27)       // 23 - 24(0)
};

vec3 getWave(vec3 worldPos)
{
    float speed1 = u_Time;
    vec3 coord1 = worldPos.xyz / textureSize(noiseTexture128, 0).x;
    coord1.x *= 3;
    coord1.x += speed1;
    coord1.z += speed1 * 0.2;
    float waveNoise1 = texture2D(noiseTexture128, coord1.xz).x;

    float speed2 = u_Time/2;
    vec3 coord2 = worldPos.xyz / textureSize(noiseTexture128, 0).x;
    coord2.x *= 0.5;
    coord2.x -= (speed2 * 0.15 + waveNoise1 * 0.05);
    coord2.z -= (speed2 * 0.7 - waveNoise1 * 0.05);
    float waveNoise2 = texture2D(noiseTexture128, coord2.xz).x;

    return waveNoise2 * 0.6 + 0.4;
}

vec3 drawWater(vec3 color, vec4 worldPos, vec4 fragToCam, vec3 normal)
{

    float wave = getWave(worldPos);
    vec3 finalColor = skyCol;
    finalColor *= wave;

    // 透射
    float cosine = dot(normalize(fragToCam.xyz), normalize(normal));
    cosine = clamp(abs(cosine), 0, 1);
    float factor = pow(1.0 - cosine, 4);
    finalColor = mix(color, finalColor, factor);

    return finalColor;
}

float ShadowCalculation(vec3 fragPosWorldSpace)
{
    // select cascade layer
    vec4 fragPosViewSpace = u_View * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0)
    {
        return 0.0;
    }
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    float bias = max(0.05 * (1.0 - dot(normal, u_LightVec)), 0.005);
    const float biasModifier = 0.5f / ((layer + 1));
    if (layer == cascadeCount)
    {
        bias *= 1 / (u_FarPlane * biasModifier);
    }
    else
    {
        bias *= 1 / (cascadePlaneDistances[layer] * biasModifier);
    }

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    // PCF
    if(layer <= 4)
    {
        int PCFRadius = 1;
        for(int x = -1 * PCFRadius; x <= PCFRadius; ++x)
        {
            for(int y = -1 * PCFRadius; y <= PCFRadius; ++y)
            {
                vec2 pcfTexCoords = projCoords.xy + vec2(x, y) * texelSize;
                float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
                shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
    }

    else
    {
//        bias = 1e-4;
//        float closestDepth = texture(shadowMap, vec3(projCoords.xy, layer)).r;
//        shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;

        shadow = 0;
    }

    return shadow;
}

float getDepth(vec3 fragPosWorldSpace)
{
    // select cascade layer
    vec4 fragPosViewSpace = u_View * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, vec3(projCoords.xy, layer)).r;

    return closestDepth;
}

vec3 getLayerCol(vec3 fragPosWorldSpace)
{
    // select cascade layer
    vec4 fragPosViewSpace = u_View * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    if(layer == 0)
    {
        return vec3(1,0,0);
    }

    if(layer == 1)
    {
        return vec3(0,1,0);
    }

    if(layer == 2)
    {
        return vec3(0,0,1);
    }
    return vec3(1,1,1);
}

float transformDepth(float x)
{
    return -1 * x * x * x + 3 * x * x - x;
}


vec3 drawSky(vec3 color, vec3 positionInViewCoord, bool isSky)
{
    float dis = isSky ? 1.f : length(positionInViewCoord.xyz) / u_FarPlane;

    // time: 0-24
    // get sky color and sun color
    int hour = int(floor(Time));
    float skyMixTerm = Time - hour;
    vec3 skyCol = mix(skyColArray[hour], skyColArray[(hour + 1) >= 24 ? 0 : (hour + 1)], skyMixTerm);
    vec3 sunCol = mix(sunColArr[hour], sunColArr[(hour + 1) >= 24 ? 0 : (hour + 1)], skyMixTerm);

    //get view space light vec
    vec3 lightVecViewSpace = u_SunDirView;
    vec3 fragDirViewSpace = vec3(u_ProjInv * vec4(TexCoords * 2 - vec2(1), 0.f, 1.f));
    fragDirViewSpace = normalize(fragDirViewSpace);
    vec3 drawSun =vec3(0);
    float disToSun = 1 - dot(lightVecViewSpace, fragDirViewSpace);
    if(disToSun < 1e-3 && isSky)
    {
        drawSun = sunCol * 2 * (1 - NightMixTerm);
    }

    // mix the sky fog and sun
    float sunMixFactor = clamp(1.0 - disToSun, 0.f, 1.f) * (1 - NightMixTerm) * HaloTerm;
    vec3 finalColor = mix(skyCol, sunCol * 0.8, pow(sunMixFactor, 4));

    return mix(color, finalColor, clamp(pow(dis, 3), 0, 1)) + drawSun;
}

void main()
{

    // retrieve data from gbuffer
    // all in the world coordinate
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 FragPosInViewSpace = vec3(u_View * vec4(FragPos, 1.f));
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    bool isWater = texture(gNormal, TexCoords).a > 0.9;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    float occlusion = texture(gSSAO, TexCoords).r;

    vec3 lightColor = vec3(1.f);
    vec3 ambient = 0.5 * lightColor;
    // then calculate lighting as usual
    // diffuse
    float diff = max(dot(normalize(u_LightVec), Normal), 0.0);
    diff = clamp(diff, 0.5f, 0.9f);
    vec3 diffuseTerm = diff * lightColor;

    //shadow
    float shadow = 1;
    if((Time >= 0 && Time < 12))
    {
        shadow = ShadowCalculation(FragPos);
    }
    float ssaoTerm = 1 - smoothstep(u_FarPlane/5, u_FarPlane/4, length(FragPosInViewSpace.xyz));
    occlusion = (1 - occlusion) * ssaoTerm;
    occlusion = isWater ? 0 : occlusion;
    vec3 lighting = (ambient * (u_useSSAO ? 1 - occlusion : 1) + (1.0 - shadow) * diffuseTerm) * Diffuse;

    vec3 finalCol = lighting;
    finalCol = drawSky(finalCol, FragPosInViewSpace, Normal == vec3(0,0,0));

    if(u_showDepth)
    {
        finalCol = vec3(getDepth(FragPos));
    }

    if(u_showLayer)
    {
        finalCol = getLayerCol(FragPos);
    }

    FragColor = vec4(finalCol, 1.0);
}
