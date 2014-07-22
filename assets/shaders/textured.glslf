varying vec2 vTexCoord;
uniform sampler2D texture_unit_0;
uniform vec4 color;
void main()
{
        gl_FragColor = color * texture2D(texture_unit_0, vTexCoord);
}
