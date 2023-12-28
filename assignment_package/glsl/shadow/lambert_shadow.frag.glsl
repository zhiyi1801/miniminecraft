#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;
uniform float u_FarPlane;
uniform float u_SunRad;
uniform float u_CamViewSize;
uniform vec3 u_LightVec;
uniform bool u_showDepth;

vec2 getFishEyeCoord(vec2 positionInNdcCoord) {
    return positionInNdcCoord / (0.15 + 0.85*length(positionInNdcCoord));
}

float random1(vec3 p) {
    return fract(sin(dot(p,vec3(127.1, 311.7, 191.999)))
                 *43758.5453);
}

float mySmoothStep(float a, float b, float t) {
    t = smoothstep(0, 1, t);
    return mix(a, b, t);
}

float cubicTriMix(vec3 p) {
    vec3 pFract = fract(p);
    float llb = random1(floor(p) + vec3(0,0,0));
    float lrb = random1(floor(p) + vec3(1,0,0));
    float ulb = random1(floor(p) + vec3(0,1,0));
    float urb = random1(floor(p) + vec3(1,1,0));

    float llf = random1(floor(p) + vec3(0,0,1));
    float lrf = random1(floor(p) + vec3(1,0,1));
    float ulf = random1(floor(p) + vec3(0,1,1));
    float urf = random1(floor(p) + vec3(1,1,1));

    float mixLoBack = mySmoothStep(llb, lrb, pFract.x);
    float mixHiBack = mySmoothStep(ulb, urb, pFract.x);
    float mixLoFront = mySmoothStep(llf, lrf, pFract.x);
    float mixHiFront = mySmoothStep(ulf, urf, pFract.x);

    float mixLo = mySmoothStep(mixLoBack, mixLoFront, pFract.z);
    float mixHi = mySmoothStep(mixHiBack, mixHiFront, pFract.z);

    return mySmoothStep(mixLo, mixHi, pFract.y);
}

float fbm(vec3 p) {
    float amp = 0.5;
    float freq = 4.0;
    float sum = 0.0;
    for(int i = 0; i < 8; i++) {
        sum += cubicTriMix(p * freq) * amp;
        amp *= 0.5;
        freq *= 2.0;
    }
    return sum;
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = getFishEyeCoord(projCoords.xy);
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    float unitLen = u_CamViewSize / textureSize(shadowMap, 0).x;
    float cotSunRad = tan(u_SunRad) < 0.01 ? 100 : 1/tan(u_SunRad);
    float sinLightFrag = dot(normalize(fs_in.Normal), normalize(u_LightVec));
    float cotLightFrag = abs(sinLightFrag) < 1e-10 ? 1e10 : sqrt(1-sinLightFrag * sinLightFrag) / sinLightFrag;
    float bias = unitLen * cotLightFrag / u_FarPlane;
    bias = clamp(bias, 1e-5f, 0.01);

    float shadow = 0.f;
    // check whether current frag pos is in shadow
    shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    // PCF
//    shadow = 0.0;
//    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
//    for(int x = -1; x <= 1; ++x)
//    {
//        for(int y = -1; y <= 1; ++y)
//        {
//            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
//            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
//        }
//    }
//    shadow /= 9.0;

    //ESM
//    float ESMFactor = 50;
//    float occluderDepth = texture(shadowMap, projCoords.xy).r;

//    shadow = exp(-1 * ESMFactor * (currentDepth - bias - occluderDepth));
//    shadow = clamp(shadow, 0, 1);
//    shadow = 1 - shadow;

    return shadow;
}

float getDepth(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = getFishEyeCoord(projCoords.xy);
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    return closestDepth;
}

float transformDepth(float x)
{
    return -1 * x * x * x + 3 * x * x - x;
}

void main()
{
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    float alpha = texture(diffuseTexture, fs_in.TexCoords).a;
    vec3 normal = normalize(fs_in.Normal);
    float shadowStrength = 0.6;
    vec3 lightColor = vec3(0.3);
    // ambient
    vec3 ambient = 1.f * lightColor;
    // Calculate the diffuse term for Lambert shading
    float diffuseTerm = dot(normalize(fs_in.Normal), normalize(u_LightVec));
    // Avoid negative lighting values
    diffuseTerm = clamp(diffuseTerm, 0, 1);
    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace);
    vec3 lighting = (ambient + (1.0 - shadow) * diffuseTerm * (1 - ambient)) * color;
    float depth = getDepth(fs_in.FragPosLightSpace);

    FragColor = vec4(u_showDepth ? vec3(depth) : lighting, alpha);
}
