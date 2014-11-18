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

#include "precompiled.h"
#include "mesh.h"

namespace fpl {

void Mesh::SetAttributes(GLuint vbo, const Attribute *attributes, int stride,
                         const char *buffer) {
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  size_t offset = 0;
  for (;;) {
    switch (*attributes++) {
      case kPosition3f:
        GL_CALL(glEnableVertexAttribArray(kAttributePosition));
        GL_CALL(glVertexAttribPointer(kAttributePosition, 3, GL_FLOAT, false,
                                      stride, buffer + offset));
        offset += 3 * sizeof(float);
        break;
      case kNormal3f:
        GL_CALL(glEnableVertexAttribArray(kAttributeNormal));
        GL_CALL(glVertexAttribPointer(kAttributeNormal, 3, GL_FLOAT, false,
                                      stride, buffer + offset));
        offset += 3 * sizeof(float);
        break;
      case kTangent4f:
        GL_CALL(glEnableVertexAttribArray(kAttributeTangent));
        GL_CALL(glVertexAttribPointer(kAttributeTangent, 3, GL_FLOAT, false,
                                      stride, buffer + offset));
        offset += 4 * sizeof(float);
        break;
      case kTexCoord2f:
        GL_CALL(glEnableVertexAttribArray(kAttributeTexCoord));
        GL_CALL(glVertexAttribPointer(kAttributeTexCoord, 2, GL_FLOAT, false,
                                      stride, buffer + offset));
        offset += 2 * sizeof(float);
        break;
      case kColor4ub:
        GL_CALL(glEnableVertexAttribArray(kAttributeColor));
        GL_CALL(glVertexAttribPointer(kAttributeColor, 4, GL_UNSIGNED_BYTE, true,
                                      stride, buffer + offset));
        offset += 4;
        break;
      case kEND:
        return;
    }
  }
}

void Mesh::UnSetAttributes(const Attribute *attributes) {
  for (;;) {
    switch (*attributes++) {
      case kPosition3f:
        GL_CALL(glDisableVertexAttribArray(kAttributePosition));
        break;
      case kNormal3f:
        GL_CALL(glDisableVertexAttribArray(kAttributeNormal));
        break;
      case kTangent4f:
        GL_CALL(glDisableVertexAttribArray(kAttributeTangent));
        break;
      case kTexCoord2f:
        GL_CALL(glDisableVertexAttribArray(kAttributeTexCoord));
        break;
      case kColor4ub:
        GL_CALL(glDisableVertexAttribArray(kAttributeColor));
        break;
      case kEND:
        return;
    }
  }
}

Mesh::Mesh(const void *vertex_data, int count, int vertex_size,
           const Attribute *format)
    : vertex_size_(vertex_size), format_(format) {
  GL_CALL(glGenBuffers(1, &vbo_));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, count * vertex_size, vertex_data,
                       GL_STATIC_DRAW));
}

Mesh::~Mesh() {
  GL_CALL(glDeleteBuffers(1, &vbo_));
  for (auto it = indices_.begin(); it != indices_.end(); ++it) {
    GL_CALL(glDeleteBuffers(1, &it->ibo));
  }
}

void Mesh::AddIndices(const unsigned short *index_data, int count, Material *mat) {
  indices_.push_back(Indices());
  auto &idxs = indices_.back();
  idxs.count = count;
  GL_CALL(glGenBuffers(1, &idxs.ibo));
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxs.ibo));
  GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(int), index_data,
                       GL_STATIC_DRAW));
  idxs.mat = mat;
}

void Mesh::Render(Renderer &renderer, bool ignore_material) {
  SetAttributes(vbo_, format_, vertex_size_, nullptr);
  for (auto it = indices_.begin(); it != indices_.end(); ++it) {
    if (!ignore_material) it->mat->Set(renderer);
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->ibo));
    GL_CALL(glDrawElements(GL_TRIANGLES, it->count, GL_UNSIGNED_SHORT, 0));
  }
  UnSetAttributes(format_);
}

void Mesh::RenderArray(GLenum primitive, int index_count,
                       const Attribute *format, int vertex_size,
                       const char *vertices, const unsigned short *indices) {
  SetAttributes(0, format, vertex_size, vertices);
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  GL_CALL(glDrawElements(primitive, index_count, GL_UNSIGNED_SHORT, indices));
  UnSetAttributes(format);
}

void Mesh::RenderAAQuadAlongX(const vec3 &bottom_left, const vec3 &top_right,
                              const vec2 &tex_bottom_left,
                              const vec2 &tex_top_right) {
  static Attribute format[] = { kPosition3f, kTexCoord2f, kEND };
  static unsigned short indices[] = { 0, 1, 2, 1, 2, 3 };
  // vertex format is [x, y, z] [u, v]:
  float vertices[] = {
    bottom_left.x(), bottom_left.y(), bottom_left.z(),
    tex_bottom_left.x(), tex_bottom_left.y(),
    top_right.x(),   bottom_left.y(), bottom_left.z(),
    tex_top_right.x(), tex_bottom_left.y(),
    bottom_left.x(), top_right.y(),   top_right.z(),
    tex_bottom_left.x(), tex_top_right.y(),
    top_right.x(),   top_right.y(),   top_right.z(),
    tex_top_right.x(), tex_top_right.y()
  };
  Mesh::RenderArray(GL_TRIANGLES, 6, format, sizeof(float) * 5,
                    reinterpret_cast<const char *>(vertices), indices);
}

// Compute normals and tangents for a mesh based on positions and texcoords.
void Mesh::ComputeNormalsTangents(NormalMappedVertex *vertices,
                                   const unsigned short *indices,
                                   int numverts,
                                   int numindices) {
  std::unique_ptr<vec3[]> binormals(new vec3[numverts]);

  // set all normals to 0, as we'll accumulate
  for (int i = 0; i < numverts; i++) {
    vertices[i].norm = mathfu::kZeros3f;
    vertices[i].tangent = mathfu::kZeros4f;
    binormals[i] = mathfu::kZeros3f;
  }
  // Go through each triangle and calculate tangent space for it, then
  // contribute results to adjacent triangles.
  // For a description of the math see e.g.:
  // http://www.terathon.com/code/tangent.html
  for (int i = 0; i < numindices; i += 3) {
    auto &v0 = vertices[indices[i + 0]];
    auto &v1 = vertices[indices[i + 1]];
    auto &v2 = vertices[indices[i + 2]];
    // The cross product of two vectors along the triangle surface from the
    // first vertex gives us this triangle's normal.
    auto q1 = vec3(v1.pos) - vec3(v0.pos);
    auto q2 = vec3(v2.pos) - vec3(v0.pos);
    auto norm = normalize(cross(q1, q2));
    // Contribute the triangle normal into all 3 verts:
    v0.norm = vec3(v0.norm) + norm;
    v1.norm = vec3(v1.norm) + norm;
    v2.norm = vec3(v2.norm) + norm;
    // Similarly create uv space vectors:
    auto uv1 = vec2(v1.tc) - vec2(v0.tc);
    auto uv2 = vec2(v2.tc) - vec2(v0.tc);
    float m = 1 / (uv1.x() * uv2.y() - uv2.x() * uv1.y());
    auto tangent = vec4((uv2.y() * q1 - uv1.y() * q2) * m, 0);
    auto binorm = (uv1.x() * q2 - uv2.x() * q1) * m;
    v0.tangent = vec4(v0.tangent) + tangent;
    v1.tangent = vec4(v1.tangent) + tangent;
    v2.tangent = vec4(v2.tangent) + tangent;
    binormals[indices[i + 0]] = binorm;
    binormals[indices[i + 1]] = binorm;
    binormals[indices[i + 2]] = binorm;
  }
  // Normalize per vertex tangent space constributions, and pack tangent /
  // binormal into a 4 component tangent.
  for (int i = 0; i < numverts; i++) {
    auto norm = vec3(vertices[i].norm);
    auto tangent = vec4(vertices[i].tangent);
    // Renormalize all 3 axes:
    norm = normalize(norm);
    tangent = vec4(normalize(tangent.xyz()), 0);
    binormals[i] = normalize(binormals[i]);
    tangent = vec4(
      // Gram-Schmidt orthogonalize xyz components:
      normalize(tangent.xyz() - norm * dot(norm, tangent.xyz())),
      // The w component is the handedness, set as difference between the
      // binormal we computed from the texture coordinates and that from the
      // cross-product:
      dot(cross(norm, tangent.xyz()), binormals[i])
    );
    vertices[i].norm = norm;
    vertices[i].tangent = tangent;
  }
}

}  // namespace fpl
