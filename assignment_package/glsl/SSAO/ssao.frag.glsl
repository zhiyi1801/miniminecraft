#version 330 core
out float FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 u_Model;
uniform mat4 u_Projection;
uniform mat4 u_View;
uniform float u_ScreenWidth;
uniform float u_ScreenHeight;
// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 64;
float radius = 5.0;

// tile noise texture over screen based on screen dimensions divided by noise size
vec2 noiseScale = vec2(u_ScreenWidth/4.0f, u_ScreenHeight/4.0f);


//void main()
//{
//    // Get input for SSAO algorithm
//    vec3 fragPos = texture(gPosition, TexCoords).xyz;
//    vec3 normal = texture(gNormal, TexCoords).rgb;
//    vec3 randomVec = texture(texNoise, TexCoords * noiseScale).xyz;
//    //randomVec = dot(normal, randomVec) < 0.99 ? randomVec : vec3(1,1,0);
//    // Create TBN change-of-basis matrix: from tangent-space to view-space
//    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
//    vec3 bitangent = cross(normal, tangent);
//    mat3 TBN = mat3(tangent, bitangent, normal);
//    // Iterate over the sample kernel and calculate occlusion factor
//    float occlusion = 0.0;
//    vec3 fragViewPos = vec3(u_View * vec4(fragPos, 1.f));
//    for(int i = 0; i < kernelSize; ++i)
//    {
//        // get sample position
//        vec3 samplePoint = TBN * samples[i]; // From tangent to world
//        samplePoint = fragPos + samplePoint * radius;
//        samplePoint = vec3(u_View * vec4(samplePoint, 1.f));

//        // project sample position (to sample texture) (to get position on screen/texture)
//        vec4 offset = vec4(samplePoint, 1.0);
//        offset = u_Projection * offset; // from world to clip-space
//        offset.xyz /= offset.w; // perspective divide
//        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

//        // get sample depth
//        float sampleDepth = -texture(gPosition, offset.xy).w; // Get depth value of kernel sample

//        // range check & accumulate
//        //float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth ));
//        float bias = 0.025;
//        if (abs(sampleDepth - samplePoint.z) < bias) {
//            occlusion += (sampleDepth - 0.01 >= samplePoint.z ? 1.0 : 0.0);
//        }
//    }
//    occlusion = 1.0 - (occlusion / kernelSize);

//    FragColor = vec3(occlusion);
//}

void main()
{
    // Get input for SSAO algorithm
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    vec3 randomVec = texture(texNoise, TexCoords * noiseScale).xyz;
    //transform to view space
    fragPos = vec3(u_View * vec4(fragPos, 1.0));
    normal = vec3(transpose(inverse(u_View * u_Model)) * vec4(normal, .0f));

    // Create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // Iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 samplePoint = TBN * samples[i]; // From tangent to view-space
        samplePoint = fragPos + samplePoint * radius;

        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePoint, 1.0);
        offset = u_Projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        // get sample depth
        if(offset.x <= 1 && offset.x >= 0 && offset.y <= 1 && offset.y >= 0)
        {
            float sampleDepth = -texture(gPosition, offset.xy).w; // Get depth value of kernel sample

            // range check & accumulate
            if(abs(samplePoint.z - sampleDepth) < 0.5)
            {
                float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth ));
                occlusion += (sampleDepth - 0.1 >= samplePoint.z ? 1.0 : 0.0) * rangeCheck;
            }
        }
    }
    occlusion = 1.0 - (occlusion / kernelSize);

    FragColor = occlusion;
}
