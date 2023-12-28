#version 400 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D shadowMap;
//uniform float near_plane;
//uniform float far_plane;

// required when using a perspective projection matrix
//float LinearizeDepth(float depth)
//{
//    float z = depth * 2.0 - 1.0; // Back to NDC
//    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
//}

vec2 getFishEyeCoord(vec2 positionInNdcCoord) {
    return positionInNdcCoord / (0.15 + 0.85*length(positionInNdcCoord));
}

void main()
{
    vec2 originalTex = getFishEyeCoord(TexCoords * 2 - vec2(1.f));
    originalTex = (originalTex + vec2(1.f)) / 2;
    //this is to show fish eye depth map
    float depthValue = texture(shadowMap, TexCoords).r;
    // this is to show original depth map
    //float depthValue = texture(shadowMap, originalTex).r;

    // FragColor = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0); // perspective
    FragColor = vec4(vec3(depthValue), 1.0); // orthographic
}
