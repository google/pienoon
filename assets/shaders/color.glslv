attribute vec4 aPosition;
uniform mat4 model_view_projection;
void main()
{
        gl_Position = model_view_projection * aPosition;
}
