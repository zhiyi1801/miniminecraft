#version 400

//This is a vertex shader. While it is called a "shader" due to outdated conventions, this file
//is used to apply matrix transformations to the arrays of vertex data passed to it.
//Since this code is run on your GPU, each vertex is transformed simultaneously.
//If it were run on your CPU, each vertex would have to be processed in a FOR loop, one at a time.
//This simultaneous transformation allows your program to run much faster, especially when rendering
//geometry with millions of vertices.

uniform mat4 u_ViewProj;    // The matrix that defines the camera's transformation.
                            // We've written a static matrix for you to use for HW2,
                            // but in HW3 you'll have to generate one yourself

in vec4 vs_Pos;             // The array of vertex positions passed to the shader
in vec4 vs_Nor;             // The array of vertex normals passed to the shader

in vec3 vs_ColInstanced;    // The array of vertex colors passed to the shader.
in vec3 vs_OffsetInstanced; // Used to position each instance of the cube

out vec4 fs_Pos;
out vec4 fs_Nor;            // The array of normals that has been transformed by u_ModelInvTr. This is implicitly passed to the fragment shader.
out vec4 fs_LightVec;       // The direction in which our virtual light lies, relative to each vertex. This is implicitly passed to the fragment shader.
out vec4 fs_Col;            // The color of each vertex. This is implicitly passed to the fragment shader.

const vec4 lightDir = normalize(vec4(0.5, 1, 0.75, 0));  // The direction of our virtual light, which is used to compute the shading of
                                        // the geometry in the fragment shader.

void main()
{
    vec4 offsetPos = vs_Pos + vec4(vs_OffsetInstanced, 0.);
    fs_Pos = offsetPos;
    fs_Col = vec4(vs_ColInstanced, 1.);                         // Pass the vertex colors to the fragment shader for interpolation

    fs_Nor = vs_Nor;

    fs_LightVec = (lightDir);  // Compute the direction in which the light source lies

    gl_Position = u_ViewProj * offsetPos;// gl_Position is a built-in variable of OpenGL which is
                                             // used to render the final positions of the geometry's vertices
}


// *********************************************************************************************************************************************

//#version 400

////This is a vertex shader. While it is called a "shader" due to outdated conventions, this file
////is used to apply matrix transformations to the arrays of vertex data passed to it.
////Since this code is run on your GPU, each vertex is transformed simultaneously.
////If it were run on your CPU, each vertex would have to be processed in a FOR loop, one at a time.
////This simultaneous transformation allows your program to run much faster, especially when rendering
////geometry with millions of vertices.

//uniform mat4 u_ViewProj;    // The matrix that defines the camera's transformation.
//                            // We've written a static matrix for you to use for HW2,
//                            // but in HW3 you'll have to generate one yourself

//in vec4 vs_Pos;             // The array of vertex positions passed to the shader
//in vec4 vs_Nor;             // The array of vertex normals passed to the shader

//in vec3 vs_ColInstanced;    // The array of vertex colors passed to the shader.
//in vec3 vs_OffsetInstanced; // Used to position each instance of the cube

//out vec4 fs_Pos;
//out vec4 fs_Nor;            // The array of normals that has been transformed by u_ModelInvTr. This is implicitly passed to the fragment shader.
//out vec4 fs_LightVec;       // The direction in which our virtual light lies, relative to each vertex. This is implicitly passed to the fragment shader.
//out vec4 fs_Col;            // The color of each vertex. This is implicitly passed to the fragment shader.

//const vec4 lightDir = normalize(vec4(0.5, 1, 0.75, 0));  // The direction of our virtual light, which is used to compute the shading of
//                                        // the geometry in the fragment shader.

//vec2 smoothF(vec2 uv)
//{
//    return uv*uv*(3.-2.*uv);
//}

//float noise(in vec2 uv)
//{
//    const float k = 257.;
//    vec4 l  = vec4(floor(uv),fract(uv));
//    float u = l.x + l.y * k;
//    vec4 v  = vec4(u, u+1.,u+k, u+k+1.);
//    v       = fract(fract(1.23456789*v)*v/.987654321);
//    l.zw    = smoothF(l.zw);
//    l.x     = mix(v.x, v.y, l.z);
//    l.y     = mix(v.z, v.w, l.z);
//    return    mix(l.x, l.y, l.w);
//}

//float fbm(const in vec2 uv)
//{
//    float a = 0.5;
//    float f = 5.0;
//    float n = 0.;
//    int it = 8;
//    for(int i = 0; i < 32; i++)
//    {
//        if(i<it)
//        {
//            n += noise(uv*f)*a;
//            a *= .5;
//            f *= 2.;
//        }
//    }
//    return n;
//}

//vec2 random2( vec2 p ) {
//    return normalize(2 * fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453) - 1);
//}

//float surflet(vec2 P, vec2 gridPoint)
//{
//    // Compute falloff function by converting linear distance to a polynomial (quintic smootherstep function)
//    float distX = abs(P.x - gridPoint.x);
//    float distY = abs(P.y - gridPoint.y);
//    float tX = 1 - 6 * pow(distX, 5.0) + 15 * pow(distX, 4.0) - 10 * pow(distX, 3.0);
//    float tY = 1 - 6 * pow(distY, 5.0) + 15 * pow(distY, 4.0) - 10 * pow(distY, 3.0);

//    // Get the random vector for the grid point
//    vec2 gradient = random2(gridPoint);
//    // Get the vector from the grid point to P
//    vec2 diff = P - gridPoint;
//    // Get the value of our height field by dotting grid->P with our gradient
//    float height = dot(diff, gradient);
//    // Scale our height field (i.e. reduce it) by our polynomial falloff function
//    return height * tX * tY;
//}

//float PerlinNoise(vec2 uv)
//{
//    // Tile the space
//    vec2 uvXLYL = floor(uv);
//    vec2 uvXHYL = uvXLYL + vec2(1,0);
//    vec2 uvXHYH = uvXLYL + vec2(1,1);
//    vec2 uvXLYH = uvXLYL + vec2(0,1);

//    return surflet(uv, uvXLYL) + surflet(uv, uvXHYL) + surflet(uv, uvXHYH) + surflet(uv, uvXLYH);
//}

//float WorleyNoise(vec2 uv)
//{
//    // Tile the space
////    uv = uv + fbm2(uv / 4) * 5.f;
//    vec2 uvInt = floor(uv);
//    vec2 uvFract = fract(uv);

//    float minDist = 1.0; // Minimum distance initialized to max.
//    float secondMinDist = 1.0;
//    vec2 closestPoint;

//    // Search all neighboring cells and this cell for their point
//    for(int y = -1; y <= 1; y++)
//    {
//        for(int x = -1; x <= 1; x++)
//        {
//            vec2 neighbor = vec2(float(x), float(y));

//            // Random point inside current neighboring cell
//            vec2 point = random2(uvInt + neighbor);

//            // Compute the distance b/t the point and the fragment
//            // Store the min dist thus far
//            vec2 diff = neighbor + point - uvFract;
//            float dist = length(diff);
//            if(dist < minDist) {
//                secondMinDist = minDist;
//                minDist = dist;
//                closestPoint = point;
//            }
//            else if(dist < secondMinDist) {
//                secondMinDist = dist;
//            }
//        }
//    }
//    float height = 0.5 * minDist + 0.5 * secondMinDist;
//    height = length(closestPoint);
////    height = height * height;
//    return height;
//}

//// I want my terrain's height to be exactly in the range:
//// [130, 150]

//float desertHeight(vec2 xz)
//{
//    float h = 0;

//    float amp = 0.5;
//    float freq = 128;
//    for(int i = 0; i < 4; ++i) {
//        vec2 offset = vec2(fbm(xz / 256), fbm(xz / 300) + 1000);
//        float h1 = PerlinNoise((xz + offset * 75) / freq);
////        float h1 = WorleyNoise(xz / freq);
////        h1 = 1. - abs(h1);
////        h1 = pow(h1, 1.5);
//        h += h1 * amp;

//        amp *= 0.5;
//        freq *= 0.5;
//    }

//    h = smoothstep(0.1, 0.25,h) * 0.9 + (0.1 * h);
//    h = floor(50 + h * 100);

//    return h;
//}

//float grassLandHeight(vec2 xz)
//{
//    float h = 0;

//    float amp = 0.5;
//    float freq = 256;
//    for(int i = 0; i < 4; ++i) {
//        vec2 offset = vec2(fbm(xz / 256), fbm(xz / 300) + 1000);
//        float h1 = PerlinNoise((xz + offset * 75) / freq);
////        float h1 = WorleyNoise(xz / freq);
////        h1 = 1. - abs(h1);
////        h1 = pow(h1, 1.5);
//        h += h1 * amp;

//        amp *= 0.5;
//        freq *= 0.5;
//    }

//    //h = smoothstep(0.1, 0.15,h) * 0.9 + (0.1 * h);
//    h = floor(75 + h * 50);

//    return h;
//}

//float testFunc(vec2 xz)
//{
////    vec2 offset = vec2(fbm(p / 256), fbm(p / 300) + 1000);
////    float h1 = PerlinNoise((p + offset * 75) / 128);

////    return h1;
//    float h = 0;

//    float amp = 0.5;
//    float freq = 128;
//    for(int i = 0; i < 4; ++i) {
//        vec2 offset = vec2(fbm(xz / 256), fbm(xz / 300) + 1000);
//        float h1 = PerlinNoise((xz + offset * 75) / freq);
////        float h1 = WorleyNoise(xz / freq);
////        h1 = 1. - abs(h1);
////        h1 = pow(h1, 1.5);
//        h += h1 * amp;

//        amp *= 0.5;
//        freq *= 0.5;
//    }

//    return h;
//}

//float biomeBlender(vec2 xz)
//{
//    return 0.5 * (PerlinNoise(xz/1024.0) + 1.f);
//}


//void main()
//{
//    vec4 offsetPos = vs_Pos + vec4(vs_OffsetInstanced, 0.);
//    vec2 xz = vs_OffsetInstanced.xz;

//    float desertH = desertHeight(xz);
//    float grassH = grassLandHeight(xz);
//    float testH = testFunc(xz);
//    float t = biomeBlender(xz);
//    t = smoothstep(0.4, 0.6, t);

//    float h = mix(desertH, grassH, t);

//    vec2 tempxz = vec2(13,59);
//    vec2 tempxz2 = xz;

////    float test = 0;
////    float amp = 0.5;
////    float freq = 128;
////    for(int i = 0; i < 1; ++i) {
////        vec2 offset = vec2(fbm(xz / 256), fbm(xz / 300) + 1000);
////        float h1 = PerlinNoise((xz + offset * 75) / freq);
//////        float h1 = WorleyNoise(xz / freq);
//////        h1 = 1. - abs(h1);
//////        h1 = pow(h1, 1.5);
////        test += h1 * amp;

////        amp *= 0.5;
////        freq *= 0.5;
////    }
////    test = smoothstep(0.1, 0.25,test) * 0.9 + (0.1 * test);

//    offsetPos.y *= desertH;
//    fs_Pos = offsetPos;
//    fs_Col = vec4(vs_ColInstanced, 1.);                         // Pass the vertex colors to the fragment shader for interpolation

//    fs_Col = t < 0.5 ? vec4(223, 214,170,255)/255.f : vec4(0.5, 0.9, 0.2, 1.0);

//    fs_Col = desertH > 60 ? vec4(199, 108, 34, 255)/255.f : fs_Col;

//    //fs_Col = (PerlinNoise(tempxz + fbm(tempxz / 256)) == PerlinNoise(xz + fbm(tempxz / 256)) && xz.x == tempxz.x && xz.y == tempxz.y && xz == tempxz && tempxz2 == tempxz && (xz.x - tempxz.x) == 0 && xz.x == 13 && xz.y == 59) ? vec4(1.) : vec4(.0f, .0f, .0f, 1.f);

//    fs_Nor = vs_Nor;

//    fs_LightVec = (lightDir);  // Compute the direction in which the light source lies

//    gl_Position = u_ViewProj * offsetPos;// gl_Position is a built-in variable of OpenGL which is
//                                             // used to render the final positions of the geometry's vertices
//}
