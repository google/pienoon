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
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  size_t offset = 0;
  for (;;) {
    switch (*attributes++) {
      case kPosition3f:
        glEnableVertexAttribArray(kAttributePosition);
        glVertexAttribPointer(kAttributePosition, 3, GL_FLOAT, false,
                              stride, buffer + offset);
        offset += 3 * sizeof(float);
        break;
      case kNormal3f:
        glEnableVertexAttribArray(kAttributeNormal);
        glVertexAttribPointer(kAttributeNormal, 3, GL_FLOAT, false,
                              stride, buffer + offset);
        offset += 3 * sizeof(float);
        break;
      case kTangent4f:
        glEnableVertexAttribArray(kAttributeTangent);
        glVertexAttribPointer(kAttributeTangent, 3, GL_FLOAT, false,
                              stride, buffer + offset);
        offset += 4 * sizeof(float);
        break;
      case kTexCoord2f:
        glEnableVertexAttribArray(kAttributeTexCoord);
        glVertexAttribPointer(kAttributeTexCoord, 2, GL_FLOAT, false,
                              stride, buffer + offset);
        offset += 2 * sizeof(float);
        break;
      case kColor4ub:
        glEnableVertexAttribArray(kAttributeColor);
        glVertexAttribPointer(kAttributeColor, 4, GL_UNSIGNED_BYTE, true,
                              stride, buffer + offset);
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
        glDisableVertexAttribArray(kAttributePosition);
        break;
      case kNormal3f:
        glDisableVertexAttribArray(kAttributeNormal);
        break;
      case kTangent4f:
        glDisableVertexAttribArray(kAttributeTangent);
        break;
      case kTexCoord2f:
        glDisableVertexAttribArray(kAttributeTexCoord);
        break;
      case kColor4ub:
        glDisableVertexAttribArray(kAttributeColor);
        break;
      case kEND:
        return;
    }
  }
}

Mesh::Mesh(const void *vertex_data, int count, int vertex_size,
           const Attribute *format)
    : vertex_size_(vertex_size), format_(format) {
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, count * vertex_size, vertex_data,
               GL_STATIC_DRAW);
}

Mesh::~Mesh() {
  glDeleteBuffers(1, &vbo_);
  for (auto it = indices_.begin(); it != indices_.end(); ++it) {
    glDeleteBuffers(1, &it->ibo);
  }
}

void Mesh::AddIndices(const int *index_data, int count, Material *mat) {
  indices_.push_back(Indices());
  auto &idxs = indices_.back();
  idxs.count = count;
  glGenBuffers(1, &idxs.ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxs.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(int), index_data,
               GL_STATIC_DRAW);
  idxs.mat = mat;
}

void Mesh::Render(Renderer &renderer, bool ignore_material) {
  SetAttributes(vbo_, format_, vertex_size_, nullptr);
  for (auto it = indices_.begin(); it != indices_.end(); ++it) {
    if (!ignore_material) it->mat->Set(renderer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->ibo);
    glDrawElements(GL_TRIANGLES, it->count, GL_UNSIGNED_INT, 0);
  }
  UnSetAttributes(format_);
}

void Mesh::RenderArray(GLenum primitive, int index_count,
                       const Attribute *format, int vertex_size,
                       const char *vertices, const int *indices) {
  SetAttributes(0, format, vertex_size, vertices);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDrawElements(primitive, index_count, GL_UNSIGNED_INT, indices);
  UnSetAttributes(format);
}

void Mesh::RenderAAQuadAlongX(const vec3 &bottom_left, const vec3 &top_right,
                              const vec2 &tex_bottom_left,
                              const vec2 &tex_top_right) {
  static Attribute format[] = { kPosition3f, kTexCoord2f, kEND };
  static int indices[] = { 0, 1, 2, 1, 2, 3 };
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

}  // namespace fpl
