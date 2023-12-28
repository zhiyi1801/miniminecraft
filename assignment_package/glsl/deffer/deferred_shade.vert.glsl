#version 460 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoords;

out vec2 TexCoords;
out float NightMixTerm;
out float Time; //0-24
out float HaloTerm;

uniform int u_CycleTime;
uniform int u_CurrentTime;
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

    gl_Position = vec4(position, 1.0f);
    TexCoords = texCoords;
}
