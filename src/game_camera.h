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

#ifndef GAME_CAMERA_H_
#define GAME_CAMERA_H_

#include <queue>
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"
#include "motive/motivator.h"

namespace fpl {
namespace pie_noon {

struct GameCameraState {
  mathfu::vec3 position;
  mathfu::vec3 target;

  GameCameraState() : position(mathfu::kZeros3f), target(mathfu::kZeros3f) {}
  GameCameraState(const mathfu::vec3& position, const mathfu::vec3& target)
      : position(position), target(target) {}

  bool operator==(const GameCameraState& rhs) const {
    return position[0] == rhs.position[0] && position[1] == rhs.position[1] &&
           position[2] == rhs.position[2] && target[0] == rhs.target[0] &&
           target[1] == rhs.target[1] && target[2] == rhs.target[2];
  }
  bool operator!=(const GameCameraState& rhs) const { return !operator==(rhs); }
};

// Defines a camera movement. These can be queued up in the GameCamera.
struct GameCameraMovement {
  GameCameraState end;
  float start_velocity;
  float time;
  motive::SplineInit init;
};

// Class that encapsilates camera motion.
class GameCamera {
 public:
  // Reset the camera to have initial position and target of 'state'.
  void Initialize(const GameCameraState& state, motive::MotiveEngine* engine);

  // Update the camera's motion. Must be called every frame.
  void AdvanceFrame(WorldTime delta_time);

  // Enqueue a motion for the camera. When a motion is complete, the next
  // motion will be executed.
  void QueueMovement(const GameCameraMovement& movement) {
    movements_.push(movement);
  }

  // Empty the motion queue.
  void TerminateMovements();

  // Terminate all movements and force the camera to a specific position.
  // Maintain the current facing direction. Useful for implementing a
  // free-cam for debugging.
  void OverridePosition(const mathfu::vec3& position);

  // Terminate all movements and force the camera to face a certain direction.
  void OverrideTarget(const mathfu::vec3& target);

  // Current position and target of the camera.
  GameCameraState CurrentState() const {
    return GameCameraState(Position(), Target());
  }

  // Current camera position.
  mathfu::vec3 Position() const;

  // Current camera target. That is, what the camera is facing.
  mathfu::vec3 Target() const;

  // Unit vector from Position() to Target().
  mathfu::vec3 Forward() const { return forward_; }

  // Unit vector to the right of the Forward().
  mathfu::vec3 Side() const { return side_; }

  // Unit vector from the top of the camera.
  // Forward(), Side(), and Up() make an orthonormal basis.
  mathfu::vec3 Up() const {
    return mathfu::vec3::CrossProduct(side_, forward_);
  }

  // Distance of the camera from its target.
  float Dist() const { return (Target() - Position()).Length(); }

 private:
  void ExecuteMovement(const GameCameraMovement& movement);

  // MotiveEngine that runs the percent_ Motivator.
  motive::MotiveEngine* engine_;

  // Percent we have moved from start_ to end_. Animated with a spline
  // Motivator.
  motive::Motivator1f percent_;

  // The start of the current camera movement. We animate from start_ to end_.
  GameCameraState start_;

  // The end of the current camera movement. We animate from start_ to end_.
  GameCameraState end_;

  // The direction the camera is facing.
  mathfu::vec3 forward_;

  // The direction to the right of the camera.
  mathfu::vec3 side_;

  std::queue<GameCameraMovement> movements_;
};

}  // pie_noon
}  // fpl

#endif  // GAME_CAMERA_H_
