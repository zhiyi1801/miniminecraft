#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

in vec4 fs_Col;
uniform sampler2D u_Sampler2D;

out vec4 out_Col;

void main()
{
    vec2 final_uv = fs_Col.xy;
    vec4 diffuseColor = texture(u_Sampler2D, final_uv);
    // Copy the color; there is no shading.
    out_Col = diffuseColor;
}
