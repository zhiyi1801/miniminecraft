#version 400 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

vec2 getFishEyeCoord(vec2 positionInNdcCoord) {
    return positionInNdcCoord / (0.15 + 0.85*length(positionInNdcCoord));
}

vec2 getOriginCoord(vec2 positionInFishEyeCoord)
{
    if(length(positionInFishEyeCoord) < 1e-10)
    {
        return vec2(0.f);
    }
    if(length(positionInFishEyeCoord) > 1.f)
    {
        return positionInFishEyeCoord;
    }

    float lenOriginal = 0.15 * length(positionInFishEyeCoord) / (1 - 0.85 * length(positionInFishEyeCoord));
    vec2 dir = positionInFishEyeCoord / length(positionInFishEyeCoord);
    return dir * lenOriginal;
}

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
//    TexCoords = getFishEyeCoord(aTexCoords * 2 - vec2(1.f));
//
//    if(length(getFishEyeCoord(getOriginCoord(vec2(0.3f,0.3f))) - vec2(0.3f,0.3f)) <= 1e-3)
//    {
//        TexCoords = aTexCoords;
//    }
}
