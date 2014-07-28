#ifndef RENDERER_H
#define RENDERER_H

namespace fpl {

using mathfu::vec2i;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;

class Renderer;

// The camera. Use the Ortho() and Perspective() methods in mathfu::Matrix
// to conveniently change the camera.
class Camera {
 public:
  Camera() : model_view_projection_(mat4::Identity()) {}

  mat4 model_view_projection_;
};

// Represents a shader consisting of a vertex and pixel shader. Also stores
// ids of standard uniforms. Use the Renderer class below to create these.
class Shader {
 public:

  Shader(GLuint program, GLuint vs, GLuint ps)
    : program_(program), vs_(vs), ps_(ps),
      uniform_model_view_projection_(-1),
      uniform_color_(-1),
      uniform_texture_unit_0(-1)
    {}

  ~Shader() {
    if (vs_) glDeleteShader(vs_);
    if (ps_) glDeleteShader(ps_);
    if (program_) glDeleteProgram(program_);
  }

  // Will make this shader active for any subsequent draw calls, and sets
  // uniforms (e.g. mvp matrix) based on current values in Renderer, if this
  // shader refers to them.
  void Set(const Renderer &renderer) const;

 private:
  friend class Renderer;

  void Initialize();

  GLuint program_, vs_, ps_;

  GLint uniform_model_view_projection_;
  GLint uniform_color_;
  GLint uniform_texture_unit_0;
};

// An array of these enums defines the format of vertex data.
enum Attribute {
  kEND = 0,     // The array must always be terminated by one of these.
  kPosition3f,
  kNormal3f,
  kTexCoord2f,
  kColor4ub
};

// A mesh instance contains a VBO and one or more IBO's.
class Mesh {
 public:
  // Initialize a Mesh by creating one VBO, and no IBO's.
  Mesh(const void *vertex_data, int count, int vertex_size,
      const Attribute *format);
  ~Mesh();

  // Create one IBO to be part of this mesh. May be called more than once.
  void AddIndices(const int *indices, int count, GLuint texture_id);

  // Render itself. A matching shader (and its uniforms) must have been set
  // before calling this.
  void Render();

  // Renders primatives using vertex and index data directly in local memory.
  // This is a convenient alternative to creating a Mesh instance for small
  // amounts of data, or dynamic data.
  static void RenderArray(GLenum primitive, int index_count,
                          const Attribute *format,
                          int vertex_size, const char *vertices,
                          const int *indices);

 private:
  static void SetAttributes(GLuint vbo, const Attribute *attributes,
                            int vertex_size, const char *buffer);
  static void UnSetAttributes(const Attribute *attributes);
  struct Indices {
    int count;
    GLuint ibo;
    GLuint texture_id;  // TODO: generalize
  };
  std::vector<Indices> indices_;
  size_t vertex_size_;
  const Attribute *format_;
  GLuint vbo_;
};

// The core of the rendering system. Deals with setting up and shutting down
// the window + OpenGL context (based on SDL), and creating/using resources
// such as shaders, textures, and geometry.
class Renderer {
 public:
  // Possible errors returned by Initialize()
  enum RendererError {
    kSuccess,
    kSDLFailedToInitialize,
    kFailedToOpenWindow,
    kCouldNotCreateOpenGLContext,
    kMissingExtensions,
    kMissingEntrypoint
  };

  // Creates the window + OpenGL context.
  RendererError Initialize(const vec2i &window_size = vec2i(800, 600),
                           const char *window_title = "");

  // Swaps frames. Call this once per frame inside your main loop.
  void AdvanceFrame(bool minimized);

  // Cleans up whatever Initialize creates.
  void ShutDown();

  // Clears the framebuffer. Call this after AdvanceFrame if desired.
  void ClearFrameBuffer(const vec4 &color);

  // Create a shader object from two strings containing glsl code.
  // Returns NULL upon error, with a descriptive message in glsl_error_.
  // Attribute names in the vertex shader should be aPosition, aNormal,
  // aTexCoord and aColor to match whatever attributes your vertex data has.
  Shader *CompileAndLinkShader(const char *vs_source, const char *ps_source);

  // Create a texture from a memory buffer containing xsize * ysize RGBA pixels.
  // Does not fail.
  GLuint CreateTexture(const uint8_t *buffer, const vec2i &size);

  // Create a texture from a memory buffer containing a TGA format file.
  // May only be uncompressed RGB or RGBA data. Returns 0 if the format is
  // not understood.
  GLuint CreateTextureFromTGAMemory(const void *tga_buf);

  Renderer() : color(1), window_size_(vec2i(0)), window_(nullptr),
               context_(nullptr) {}
  ~Renderer() { ShutDown(); }

 private:
  GLuint CompileShader(GLenum stage, GLuint program, const GLchar *source);

 public:
  Camera camera;
  vec4 color;
  vec2i window_size_;
  std::string glsl_error_;

 private:
  SDL_Window *window_;
  SDL_GLContext context_;
};

}  // namespace fpl

#endif  // RENDERER_H
