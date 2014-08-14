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

attribute vec4 aPosition;
attribute vec2 aTexCoord;
varying vec2 vTexCoord;
uniform mat4 model_view_projection;
uniform mat4 model;  // object to world space transform
uniform vec3 light_pos;  // in world space

void main()
{
  // Transform position to world space, since we need to project it to the
  // ground plane from the light, both in world space.
  vec3 world_pos = (model * aPosition).xyz;
  // Vector towards the the vertex.
  vec3 to_vert = normalize(world_pos - light_pos);
  // Project vertex onto the ground by extending the vector by the correct
  // length:
  vec3 world_pos_on_ground = world_pos + to_vert * (world_pos.y / -to_vert.y);
  // Place shadow slightly above ground to avoid Z-fighting:
  vec3 ground_offset= vec3(0.0, 0.01, 0.1);
  gl_Position = model_view_projection *
                vec4(world_pos_on_ground + ground_offset, 1.0);
  vTexCoord = aTexCoord;
}
