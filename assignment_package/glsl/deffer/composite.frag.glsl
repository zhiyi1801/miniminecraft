#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in float NightMixTerm;
in float Time; //0-24
in float HaloTerm;
in vec3 skyCol;
in vec3 sunCol;

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
uniform mat4 u_ViewInv;
uniform mat4 u_Proj;
uniform mat4 u_ProjInv;
uniform vec3 u_SunDirView;

uniform bool u_useSSAO;
uniform bool u_EyeInWater;

uniform float u_FarPlane;
uniform float u_NearPlane;
uniform float u_Time;

layout (std140, binding = 0) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;   // number of frusta - 1

float screenDepthToLinerDepth(float screenDepth) {
    return 2 * u_NearPlane * u_FarPlane / ((u_FarPlane + u_NearPlane) - screenDepth * (u_FarPlane - u_NearPlane));
}

//calculate underwater fadeout term for shadow
// d0 depth of all block
// d1 depth without water
float getUnderWaterFadeOut(float d0, float d1, vec4 fragToCam, vec3 normal)
{
    d0 = screenDepthToLinerDepth(d0);
    d1 = screenDepthToLinerDepth(d1);

    float cosine = dot(normalize(fragToCam.xyz), normalize(mat3(u_View) * normal));
    cosine = clamp(abs(cosine), 0, 1);

    return clamp(1.0 - (d1 - d0) * cosine * 0.05, 0, 1);
}

float getWave(vec3 worldPos)
{
    float speed1 = u_Time;
    vec3 coord1 = worldPos.xyz / textureSize(noiseTexture128, 0).x;
    coord1.x *= 3;
    coord1.x += speed1 * 0.3;
    coord1.z += speed1 * 0.2;
    float waveNoise1 = texture2D(noiseTexture128, coord1.xz).x;

    float speed2 = u_Time/2;
    vec3 coord2 = worldPos.xyz / textureSize(noiseTexture128, 0).x;
    coord2.x *= 0.5;
    coord2.x -= (speed2 * 0.15 + waveNoise1 * 0.05);
    coord2.z -= (speed2 * 0.05 - waveNoise1 * 0.05);
    float waveNoise2 = texture2D(noiseTexture128, coord2.xz).x;

    return waveNoise2 * 0.6 + 0.4;
}

float getCaustics(vec3 worldPos) {

    //noise 1
    float speed1 = 10 * u_Time / textureSize(noiseTexture128, 0).x;
    vec3 coord1 = worldPos.xyz / textureSize(noiseTexture128, 0).x;
    coord1.x *= 4;
    coord1.x += speed1*2 + coord1.z;
    coord1.z -= speed1;
    float noise1 = texture2D(noiseTexture128, coord1.xz).x;
    noise1 = noise1*2 - 1.0;

    // noise2
    float speed2 =  10 * u_Time / textureSize(noiseTexture128, 0).x;
    vec3 coord2 = worldPos.xyz / textureSize(noiseTexture128, 0).x;
    coord2.z *= 4;
    coord2.z += speed2*2 + coord2.x;
    coord2.x -= speed2;
    float noise2 = texture2D(noiseTexture128, coord2.xz).x;
    noise2 = noise2*2 - 1.0;

    return noise1 + noise2;
}

vec3 drawWater(vec3 color, vec4 worldPos, vec4 fragToCam, vec3 normal, float shadow, float alpha)
{

    //float wave = getWave(vec3(worldPos)); // this is water wave directly use noise
    //wave = 1;                             // the new implementation is to modify normal in water gbuffer

    //when alpha is small, the fresnel color should be more close to
    vec3 finalColor = alpha > 0.8 ? skyCol : mix(vec3(0.9), skyCol, alpha / 0.8);
    //finalColor *= wave;

    // specular
    vec3 halfVec = normalize(normalize(mat3(u_View) * u_LightVec) + normalize(vec3(fragToCam)));
    float specularTerm = dot(halfVec, normal);

    //compute the fresnel term
    float cosine = dot(normalize(fragToCam.xyz), normalize(mat3(u_View) * normal));
    cosine = clamp(abs(cosine), 0, 1);
    float factor = pow(1.0 - cosine, 4);
    finalColor = mix(color * 0.8, finalColor * 0.8, factor);

    return finalColor + vec3(0.8) * pow(max(0, specularTerm), 128) * (1 - NightMixTerm) * (1 - shadow);
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

vec3 getShadow(vec3 color, vec4 worldPos)
{
    float shadow = ShadowCalculation(vec3(worldPos));
    vec4 viewPos = u_View * worldPos;
    float dis = viewPos.z / u_FarPlane;
    float strength = 0.6f;
    float shadowStrength = strength * 0.6 * (1 - dis) * (1 - 0.6 * NightMixTerm);
    vec3 ret = color * 1 - shadow * shadowStrength;

    return ret;
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

int getLayer(vec3 fragPosWorldSpace)
{
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

    return layer;
}


float transformDepth(float x)
{
    return -1 * x * x * x + 3 * x * x - x;
}


vec3 drawSky(vec3 color, vec3 positionInViewCoord, bool isSky)
{
    float dis = isSky ? 1.f : length(positionInViewCoord.xyz) / u_FarPlane;

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
    // coordinate with water
    float depth0 = texture2D(depthAll, TexCoords.xy).r;
    vec4 posInNdc0 = vec4(TexCoords.xy*2-1, depth0*2-1, 1);
    vec4 posInClip0 = u_ProjInv * posInNdc0;
    vec4 posInView0 = vec4(posInClip0.xyz / posInClip0.w, 1.f);
    vec4 posInWorld0 = u_ViewInv * posInView0;

    // coordinate without without water
    float depth1 = texture2D(depthNoWater, TexCoords.xy).r;
    vec4 posInNdc1 = vec4(TexCoords.xy*2-1, depth1*2-1, 1);
    vec4 posInClip1 = u_ProjInv * posInNdc1;
    vec4 posInView1 = vec4(posInClip1.xyz / posInClip1.w, 1.f);
    vec4 posInWorld1 = u_ViewInv * posInView1;

    // retrieve data from gbuffer
    // all in the world coordinate
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 FragPosInViewSpace = vec3(u_View * vec4(FragPos, 1.f));
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    bool isWater = texture(gNormal, TexCoords).a > 0.9 && Normal != vec3(0,0,0);
    bool isInWater = (depth1>depth0)?(true):(false);
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float alpha = texture(gAlbedoSpec, TexCoords).a;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    float occlusion = texture(gSSAO, TexCoords).r;

    vec3 lightColor = vec3(1.f);
    vec3 ambient = 0.5 * lightColor;
    // then calculate lighting as usual
    // diffuse
    float diff = max(dot(normalize(u_LightVec), Normal), 0.0);
    if(isWater)
    {
        diff = clamp(diff, 0.5, 1.f);
    }
    vec3 diffuseTerm = diff * lightColor;

    //shadow
    float shadow = 1;
    float underWaterFadeOut = getUnderWaterFadeOut(depth0, depth1, -posInView0, Normal);
    if((Time >= 0 && Time < 12))
    {
        shadow = ShadowCalculation(vec3(posInWorld1));
    }
    //shadow *= underWaterFadeOut;
    shadow *= isWater ? (1 -alpha) : 1;
    float ssaoTerm = 1 - smoothstep(u_FarPlane/5, u_FarPlane/4, length(FragPosInViewSpace.xyz));
    occlusion = (1 - occlusion) * ssaoTerm;
    occlusion = isWater ? 0 : occlusion;
    vec3 lighting = (u_useSSAO ? 1 - occlusion : 1) * (ambient  + (1.0 - shadow) * diffuseTerm) * Diffuse;
    vec3 finalCol = lighting;

    float caustics = getCaustics(vec3(posInWorld1));
    caustics = 0;
    //Caustics
    if(isInWater || u_EyeInWater)
    {
        finalCol.rgb *= (1.0 + caustics * 0.1);
    }

    if(isWater)
    {
        if(abs(screenDepthToLinerDepth(depth0) - screenDepthToLinerDepth(depth1)) < 0.3)
        {
            finalCol = clamp(vec3(0.8,0.8,0.8) * (1 - length(posInView1) / u_FarPlane) * (1 - shadow * 0.8), vec3(0.5), vec3(0.8));
        }
        else
        {
            finalCol = drawWater(finalCol, vec4(FragPos, 1.f), vec4(-1.f * FragPosInViewSpace, 1.f), Normal, shadow, alpha);
        }
    }

    finalCol = drawSky(finalCol, FragPosInViewSpace, Normal == vec3(0,0,0));

    FragColor = vec4(finalCol, 1.0);
}
