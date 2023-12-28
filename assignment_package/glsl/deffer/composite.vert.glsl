#version 460 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoords;

out vec2 TexCoords;
out float NightMixTerm;
out float Time; //0-24
out float HaloTerm;
out vec3 skyCol;
out vec3 sunCol;

uniform int u_CycleTime;
uniform int u_CurrentTime;
uniform float u_Time;

vec3 sunColArray[24] = {
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

void main()
{
    Time = (float(u_CurrentTime) / u_CycleTime) * 24;

    // NightMixTerm
    NightMixTerm = 0;  // day
    if(12<Time && Time < 13) {
        NightMixTerm = 1.0 - (13.f - Time); // at dusk
    }
    else if(13 <= Time && Time<=23) {
        NightMixTerm = 1.0;    // night
    }
    else if(23 < Time) {
        NightMixTerm = (24 - Time);   // at dawn
    }

    HaloTerm = 1;
    if(Time > 0 && Time < 6)
    {
        HaloTerm = 1 - Time / 6.f;
    }
    else if(Time >= 6 && Time < 12)
    {
        HaloTerm = (Time - 6) / 6.f;
    }
    HaloTerm = clamp(HaloTerm, 0.8f, 0.99f);

    int hour = int(floor(u_Time));
    float mixTerm = u_Time - hour;
    skyCol = mix(skyColArray[hour], skyColArray[(hour + 1) == 24 ? 0 : hour + 1], mixTerm);
    sunCol = mix(sunColArray[hour], sunColArray[(hour + 1) == 24 ? 0 : hour + 1], mixTerm);

    gl_Position = vec4(position, 1.0f);
    TexCoords = texCoords;
}
