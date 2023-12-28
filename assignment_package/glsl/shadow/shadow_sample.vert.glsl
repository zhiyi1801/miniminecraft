#version 330 core
in vec4 vs_Pos;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;

vec2 getFishEyeCoord(vec2 positionInNdcCoord) {
    return positionInNdcCoord / (0.15 + 0.85*length(positionInNdcCoord.xy));
}

void main()
{
    gl_Position = u_LightSpaceMatrix * u_Model * vs_Pos;
    gl_Position.xy = getFishEyeCoord(gl_Position.xy);
}
