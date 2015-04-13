// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FPL_MESH_H
#define FPL_MESH_H

#include "material.h"

namespace fpl {

using mathfu::vec2_packed;
using mathfu::vec3_packed;
using mathfu::vec4_packed;

class Renderer;

// An array of these enums defines the format of vertex data.
enum Attribute {
  kEND = 0,  // The array must always be terminated by one of these.
  kPosition3f,
  kNormal3f,
  kTangent4f,
  kTexCoord2f,
  kColor4ub
};

// A vertex definition specific to normalmapping.
// TODO: we ideally shouldn't have hardcoded structs like this,
// instead use Attributes
// We use the _packed versions to ensure SIMD doesn't ruin the layout.
struct NormalMappedVertex {
  vec3_packed pos;
  vec2_packed tc;
  vec3_packed norm;
  vec4_packed tangent;
};

// A mesh instance contains a VBO and one or more IBO's.
class Mesh {
 public:
  // Initialize a Mesh by creating one VBO, and no IBO's.
  Mesh(const void *vertex_data, int count, int vertex_size,
       const Attribute *format);
  ~Mesh();

  // Create one IBO to be part of this mesh. May be called more than once.
  void AddIndices(const unsigned int *indices, int count, Material *mat);

  // Render itself. Uniforms must have been set before calling this.
  void Render(Renderer &renderer, bool ignore_material = false);

  // Get the material associated with the Nth IBO.
  Material *GetMaterial(int i) { return indices_[i].mat; }

  // Renders primatives using vertex and index data directly in local memory.
  // This is a convenient alternative to creating a Mesh instance for small
  // amounts of data, or dynamic data.
  static void RenderArray(GLenum primitive, int index_count,
                          const Attribute *format, int vertex_size,
                          const char *vertices, const unsigned int *indices);

  // Convenience method for rendering a Quad. bottom_left and top_right must
  // have their X coordinate be different, but either Y or Z can be the same.
  static void RenderAAQuadAlongX(const vec3 &bottom_left, const vec3 &top_right,
                                 const vec2 &tex_bottom_left = vec2(0, 0),
                                 const vec2 &tex_top_right = vec2(1, 1));

  // Convenience method for rendering a Quad with nine patch settings.
  // In the patch_info, the user can define nine patch settings
  // as vec4(x0, y0, x1, y1) where
  // (x0,y0): top-left corner of stretchable area in UV coordinate.
  // (x1,y1): bottom-right corner of stretchable area in UV coordinate.
  static void RenderAAQuadAlongXNinePatch(const vec3 &bottom_left,
                                          const vec3 &top_right,
                                          const vec2i &texture_size,
                                          const vec4 &patch_info);

  // Compute normals and tangents given position and texcoords.
  static void ComputeNormalsTangents(NormalMappedVertex *vertices,
                                     const unsigned int *indices,
                                     int numverts, int numindices);

  enum {
    kAttributePosition,
    kAttributeNormal,
    kAttributeTangent,
    kAttributeTexCoord,
    kAttributeColor
  };

  // Compute the byte size for a vertex from given attributes.
  static size_t VertexSize(const Attribute *attributes);

 private:
  static void SetAttributes(GLuint vbo, const Attribute *attributes,
                            int vertex_size, const char *buffer);
  static void UnSetAttributes(const Attribute *attributes);
  struct Indices {
    int count;
    GLuint ibo;
    Material *mat;
  };
  std::vector<Indices> indices_;
  size_t vertex_size_;
  const Attribute *format_;
  GLuint vbo_;
};

}  // namespace fpl

#endif  // FPL_MESH_H
