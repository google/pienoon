attribute vec4 aPosition;
attribute vec2 aTexCoord;
varying vec2 vTexCoord;
uniform mat4 model_view_projection;
void main()
{
        gl_Position = model_view_projection * aPosition;
        vTexCoord = aTexCoord;
}
