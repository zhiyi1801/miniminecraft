#version 460 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2DArray shadowMap;

uniform vec3 u_LightVec;
uniform float u_FarPlane;

uniform mat4 u_View;

uniform bool u_showDepth;

uniform bool u_showLayer;

layout (std140, binding = 0) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;   // number of frusta - 1

const vec3 skyCol = vec3(0.37f, 0.74f, 1.0f);

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
    vec3 normal = normalize(fs_in.Normal);
    float bias = max(0.05 * (1.0 - dot(normal, u_LightVec)), 0.005);
    const float biasModifier = 0.5f / (layer + 1);
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
    if(layer <= 2)
    {
        int PCFRadius = 1;
        for(int x = -1 * PCFRadius; x <= PCFRadius; ++x)
        {
            for(int y = -1 * PCFRadius; y <= PCFRadius; ++y)
            {
                vec2 pcfTexCoords = projCoords.xy + vec2(x, y) * texelSize;
//                if (pcfTexCoords.x > 0.01 && pcfTexCoords.x < 0.99 && pcfTexCoords.y > 0.01 && pcfTexCoords.y < 0.99)
//                {
                float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
                shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
//                }
            }
        }
        shadow /= 9.0;
    }

    else
    {
        bias = 5e-5;
        float closestDepth = texture(shadowMap, vec3(projCoords.xy, layer)).r;
        shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
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

void main()
{
    vec2 texCoords = fs_in.TexCoords;
    vec3 color = texture(diffuseTexture, texCoords).rgb;
    float alpha = texture(diffuseTexture, texCoords).a;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(1.f);
    // ambient
    vec3 ambient = 0.5 * lightColor;
    // diffuse
    float diff = max(dot(normalize(u_LightVec), normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPos);
    vec3 lighting = (ambient + (1.0 - shadow) * diffuse * (0.8)) * color;

    vec3 finalCol = lighting;

    if(u_showDepth)
    {
        finalCol = vec3(getDepth(fs_in.FragPos));
    }

    if(u_showLayer)
    {
        finalCol = getLayerCol(fs_in.FragPos);
    }

    FragColor = vec4(finalCol, alpha);
}
