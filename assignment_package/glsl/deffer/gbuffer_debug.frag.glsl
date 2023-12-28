#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform int u_Layer;

void main()
{
    // Retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;

    vec3 finalCol = vec3(0.f);

    if(u_Layer == 0)
    {
        finalCol = FragPos;
    }
    else if(u_Layer == 1)
    {
        finalCol = Normal;
    }
    else if(u_Layer == 2)
    {
        finalCol = Diffuse;
    }
    else if(u_Layer == 3)
    {
        finalCol = vec3(Specular);
    }

    FragColor = vec4(finalCol, 1.0);
}
