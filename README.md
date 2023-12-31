MiniMinecraft
================

**University of Pennsylvania, CIS 560 final project**


<div align="center">
    <img src="image\scene.png?raw=true" width="80%"/>

</div>


### Terrain Generation
- Perlin Noise and Worley Noise

    In this part, I use Perlin Noise and Worley Noise to generate terrain. Perlin noise is spatially continuous and has a certain smoothness. This means that adjacent points have noise values ​​that are numerically close, creating a smooth, cloud-like pattern. This smoothness is achieved through interpolation algorithms.
<table>
    <tr>
        <th>Perlin Noise</th>
        <th>Worley Noise</th>
    </tr>
    <tr>
        <th><img src="image\Perlin_noise_example.png?raw=true" /></th>
        <th><img src="image\Worley.jpg?raw=true" /></th>
    </tr>
</table>

- Multithreading Generation
    In Minecraft, the terrain is segmented into sections known as "chunks". Creating these chunks individually can be a time-consuming process. To enhance efficiency, I utilize multithreading, allowing for the concurrent generation of multiple chunks. This approach significantly speeds up the terrain construction process.

### Cascaded Shadow Maps
I implement CSM to create shadow.

### SSAO
<table>
    <tr>
        <th>SSAO on</th>
        <th>SSAO off</th>
    </tr>
    <tr>
        <th><img src="image\SSAO_on.png?raw=true" /></th>
        <th><img src="image\SSAO_off.png?raw=true" /></th>
    </tr>
</table>

### Defferred Rendering
- **Basic Principle**
    The workflow of Deferred Rendering typically divides into two main phases:

    **Geometry Rendering Phase (G-Buffer Phase):**
    - During this phase, the scene's geometric information such as position, normals, diffuse colors, specular colors, etc., are rendered into a set of textures, collectively known as the G-Buffer (Geometry Buffer).
    - This step does not involve any lighting calculations.  

    **Shading Phase:**
    - In this phase, the data from the G-Buffer is used for actual lighting calculations.
    Each light source is considered individually, with its effects calculated and applied to the scene.
    - This stage usually involves calculating lighting for each pixel within the screen space.
    <br/>
    In my implementation, the rendering process is structured into multiple distinct passes, each dedicated to handling different aspects of the scene. Specifically, I employ two primary passes — the terrain pass and the water pass — to render opaque and transparent objects, respectively. This is followed by a shadow pass and SSAO pass, designed to generate the shadow map and ambient occlusion effect. Ultimately, all these textures are utilized in the final stage to accurately compute lighting, ensuring a cohesive and visually appealing representation of both opaque and transparent elements, as well as realistic shadow effects.

<table>
    <tr>
        <th>normal map in terrain pass</th>
        <th>color map in terrain pass</th>
    </tr>
    <tr>
        <th><img src="image\normal_map_terrainpass.png?raw=true" /></th>
        <th><img src="image\color_map_terrainpass.png?raw=true" /></th>
    </tr>
    <tr>
        <th>normal map in water pass</th>
        <th>color map in water pass</th>
    </tr>
    <tr>
        <th><img src="image\normal_map_waterpass.png?raw=true" /></th>
        <th><img src="image\color_map_waterpass.png?raw=true" /></th>
    </tr>
    <tr>
        <th colspan="2">final image</th>
    </tr>
    <tr>
        <th colspan="2"><img src="image\final.png?raw=true" width="60%"/></th>
    </tr>
</table>

### Frustum Culling
I implemented frustum culling according to this article: [Frustum Culling](https://www.jianshu.com/p/b42b99c8ed73)