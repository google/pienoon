varying vec2 vTexCoord;
varying vec3 vObjectSpacePosition;
varying vec3 vTangentSpaceLightVector;
varying vec3 vTangentSpaceCameraVector;
uniform sampler2D texture_unit_0;   //texture
uniform sampler2D texture_unit_1;   //normalmap
uniform vec4 color;

void main(void)
{
    //these really need to be uniforms.
    vec3 ambient_material = vec3(0.6, 0.6, 0.6);
    vec3 diffuse_material = vec3(0.7, 0.7, 0.7);
    vec3 specular_material = vec3(0.3, 0.3, 0.3);
    float shininess = 32.0;

    vec4 texture_color =  texture2D(texture_unit_0, vTexCoord);
    // We only render pixels if they are at least somewhat opaque.
    // This will still lead to aliased edges if we render
    // in the wrong order, but leaves us the option to render correctly
    // if we sort our polygons first.
    if (texture_color.a < 0.1)
      discard;
    texture_color *= color;

    // Extract the perturbed normal from the texture:
    vec3 tangent_space_normal =
      texture2D(texture_unit_1, vTexCoord).yxz * 2.0 - 1.0;

    vec3 N = tangent_space_normal;

    // Standard lighting math:
    vec3 L = normalize(vTangentSpaceLightVector);
    vec3 E = normalize(vTangentSpaceCameraVector);
    vec3 H = normalize(L + E);
    float df = abs(dot(N, L));  // change these abs() to max(0.0, ...
    float sf = abs(dot(N, H));  // to make the facing matter.
    sf = pow(sf, shininess);

    vec3 lighting = ambient_material +
        df * diffuse_material +
        sf * specular_material;
    gl_FragColor = vec4(lighting, 1) * texture_color;
}

