Modifying Assets {#pie_noon_guide_assets}
================

### Overview

The game has many data driven components allowing designers the tweak the
game mechanics and content creators to add assets.

Game data is specified by [JSON][] files in `pie_noon/src/rawassets` which are
converted to binary and read in-game using [Flatbuffers][].  Each [JSON][]
file in `pie_noon/src/rawassets` is described by a [Flatbuffers schema][] in
`pie_noon/src/flatbufferschemas`.  Each [Flatbuffers schema][] file contains
descriptions of each field (prefixed by `//`) in associated [JSON][] files.

The following table maps each set of [JSON][] files in `src/rawassets` to
[Flatbuffers][] schemas in `src/flatbufferschemas`:

| JSON File(s)                       | Flatbuffers Schema                |
|------------------------------------|-----------------------------------|
| `buses.json`                       | `buses.fbs`                       |
| `character_state_machine_def.json` | `character_state_machine_def.fbs` |
| `config.json`                      | `config.fbs`                      |
| `materials\*.json`                 | `materials.fbs`                   |
| `rendering_assets.json`            | `rendering_assets.fbs`            |
| `sound_assets.json`                | `sound_assets.fbs`                |
| `sounds\*.json`                    | `sound.fbs`                       |

### Building

The game loads assets from the `assets` directory.  In order to convert data
into a format suitable for the game runtime, assets are built from a python
script which requires:

*   [Python][] to run the asset build script. [Windows][] users may need to
    install [Python][] and add the location of the python executable to the PATH
    variable.  For example in Windows 7:
    *   Right click on `My Computer`, select `Properties`.
    *   Select `Advanced system settings`.
    *   Click `Environment Variables`.
    *   Find and select `PATH`, click `Edit...`.
    *   Add `;%PATH_TO_PYTHONEXE%` where `%PATH_TO_PYTHONEXE%` is the location
        of the python executable.
*   `flatc` ([Flatbuffers compiler][]) to convert [JSON][] text files to .bin
    files in the `pie_noon/assets` directory.
*   [cwebp][] is required to convert `png` images to the [webp][] format.
    Install [cwebp][] by downloading the libwebp archive for your operating
    system (see [WebP Precompiled Utilities][]) unpack somewhere on your
    system and add the directory containing the [cwebp][] binary to the `PATH`
    variable.

After modifying the data in the `pie_noon/src/rawassets` directory, the assets
need to be rebuilt by running the following command:

    cd pie_noon
    python scripts/build_assets.py

Each file under `src/rawassets` will produce a corresponding output file in
`assets`.  For example, after running the asset build
`assets/config.bin` will be generated from `src/rawassets/config.json`.

### Game Configuration

Global configuration options for the game are specified by data in
`src/rawassets/config.json` which uses the `Config` table of the
[Flatbuffers][] schema `src/flatbufferschema/config.fbs`.  The following
sections describe the most used subset of the configuration options.

#### Characters

The number of characters in the game is currently controlled by the
`character_count` field.  Characters are arranged in the scene using the
`character_arrangements` table which specifies the location of characters
based upon the number of characters referenced in each entry of the table.

#### Game Mechanics

`character_health` controls the initial health of each player when the
game starts.

Pies speed is controlled by `pie_flight_time`.  The animation of pie flight
is controlled using `pie_arc_height`, `pie_src_height_variance`,
`pie_initial_height`, `pie_rotations`, `pie_rotation_variance`,
`pie_initial_angle` and `pie_target_angle`.

#### Camera

The initial position of the camera is set via the `camera_position` and will
be facing the location specified by `camera_target`.  The camera's field of
view is controlled using `viewport_angle`, `viewport_aspect_ratio` and
`viewport_near_plane`, `viewport_far_plane`.

#### Lighting

`light_positions` allows declaration of multiple light-sources, but only the
first entry is used by the game.  The single light is omni-directional.

#### Visuals

The scene is composed of a range of objects called renderables.  The game
has a discrete number of renderable objects described by the `RenderableId`
enumeration in `src/flatbufferschemas/pie_noon_common.fbs`.  `RenderableId`
values are used to associate game objects with renderable objects.

All renderables in the game are billboards (quads) which can be rendered with
different materials.  Each renderable (`RenderableDef`) associates properties
of a billboard with a `RenderableId`.  The `RenderableDef` is used to specify
the materials (textures + shader) used to render a billboard.
Where `cardboard_front` the the billboard rendered in the foreground and
`cardboard_back` is rendered behind the foreground material to provide the
illusion of depth.  The materials used for popsicle sticks (rendered if
`stick` is true) are configured using `stick_front` and `stick_back`.

##### Characters

The renderables used for characters are configured using the
*Character State Machine* (see section below).

##### Character Accessories

Character accessories (`pie_noonter_accessories` and `health_accessories`) are
rendered next to each character.  For example, the health display is rendered
alongside the character to illustrate the characters remaining health.

##### Props

Props (`Prop`) are static billboards that are positioned in the scene with
positions relative to world coordinates.  Props are configured using the
`props` field and are unconditionally rendered.

##### Materials

Each renderable must be associated with a material which specifies how the
renderable is rendered.  Material [JSON][] files are located in
`src/rawassets/materials` and use the `src/flatbufferschemas/materials.fbs`
[Flatbuffers][] schema.

Fields that reference materials (`cardboard_front`, `cardboard_back`,
`stick_front` and `stick_back`) specify the path of the material .bin file
*generated* from the [JSON][] in `src/rawassets/materials`.  For example,
to use `src/rawassets/materials/pie_small.json` the designer should specify
`materials/pie_small.bin` string.

The `shader_basename` references a vertex, fragment shader pair from
`assets/shaders` to use to render the material.  For example, the string
`shaders/textured` will cause the game to load the
`assets/shaders/textured.glslf` fragment shader and the
`assets/shaders/textured.glslv` vertex shader.

Textures are associated with a material using `texture_filenames` where the
filenames - in a similar fashion to shader paths - are relative to the
`assets` directory.  Each material can be associated with multiple textures,
where the use of each texture depends upon the shader associated wih the
material.

##### Shaders

###### shaders/color

`shaders/color` does not take texture input and simply renders polygons using
vertex colors specified by the game.

###### shaders/lit_textured_normal

`shaders/lit_textured_normal` is a generic shader for rendering polygons with
a normal map.  This uses the first texture specified in the `texture_filenames`
list as the diffuse texture and the second texture as a normal map.  Ambient
and specular lighting of the material is controlled by the normal map and the
light position in the world (see `light_positions` in
`src/flatbufferschemas/config.fbs`).

###### shaders/simple_shadow

`shaders/simple_shadow` projects a shadow onto the ground plane (X-Z plane)
away from the light position (see `light_positions` in
`src/flatbufferschemas/config.fbs`).

##### shaders/textured

`shaders/textured` simply renders the first texture in `texture_filenames`
with a color tint - supplied by the game runtime - with no lighting.

### Audio

`SoundId` values in `src/flatbufferschemas/pie_noon_common.fbs` are used to
associate game events (states) in the *Character State Machine* with sounds.

`src/rawassets/sound_assets.json` associates `SoundId` values with sounds
(`SoundDef`, see `src/flatbufferschemas/sound.fbs`).  Each entry in the
`sounds` list references a .bin file in the `assets` directory generated from
a [JSON][] file in `src/rawassets/sounds`.  For example, to load
`src/rawassets/sounds/throw_pie.json` the `sounds` list should reference
`sounds/throw_pie.bin`.

Each sound (`SoundDef`) can reference a set of audio samples, where each
filename references a sample in the `assets` directory.  For example, to load
`assets/sounds/smack01.wav` the string `sounds/smack01.wav` should be added
to the `filename` entry of an `audio_sample` in the `audio_sample_set` list.

### Character State Machine

The character state machine is `Finite-state machine` described by the
`CharacterStateMachineDef` from the
`src/flatbufferschemas/character_state_machine_def.fbs` schema and configured
using `src/rawassets/character_state_machine_def.json`.

#### Transitions

All characters in the game start in the state specified by `initial_state`
in the `CharacterStateMachineDef`.  Transitions from each state
(`CharacterState`) are primarily dependent upon `transitions` (`Transition`).
When the `condition` of a `Transition` is satisified.

For example, the following declares two states `Loading_1` and `Throwing`:

    {
      "states": [
        {
          "id": "Loading_1",
          "transitions": [
            {
              "target_state": "Throwing",
              "condition": {
                "logical_inputs": "ThrowPie"
              }
            },
          ]
        },
        {
          "id": "Throwing",
          "transitions": [
            {
              "target_state": "Loading_1",
              "condition": {
                "logical_inputs": "AnimationEnd"
              }
            },
          ]
        }
      ]
    }

The state machine will leave the `Loading_1` state and enter the `Throwing`
state when the input event `ThrowPie` is received by the character.
The `ThrowPie` event is usually mapped to a button on a game controller
(see the *Controls* section).  When the animation associated with the
`Throwing` state is complete `AnimationEnd` is fired and the state machine
transitions to the `Loading_1` state.

Conditions are not limited to listening for a single input event. To make a
condition depend on two or more inputs, just list all input events in a list
separated by spaces.  For example, `"Throwing JustHit"` would be true only if
the player pressed the `Throwing` button as the character is hit.

Additionally, conditions can be configured to only be valid for a certain time
period by using the `time` and `end_time` fields.  Times are specified in
milliseconds.  For example, if you only want to allow the character to throw
after half a second, you could add `"time": 500` to the list of conditions.

Conditions are evaluated in the order they are listed.  If more the one
condition is true, the first transition in the list will be followed.

#### Events

The pallet of events is described by `LogicalInputs` in
`src/flatbufferschemas/character_state_machine_def.fbs`.  Events map to
controller input, gameplay events and the state machine's animation
(timeline) state.

| Event        | Type           | Cause                                     |
|--------------|----------------|-------------------------------------------|
| ThrowPie     | Input          | Throw button was pressed.                 |
| Deflect      | Input          | Shield / block button was pressed.        |
| Left         | Input          | Left turn button was pressed.             |
| Right        | Input          | Right turn button was pressed.            |
| JustHit      | Game Event     | Character hit by a pie.                   |
| NoHealth     | Game Event     | Character health reduced to zero.         |
| AnimationEnd | Animation      | Timeline has reached the `end_time`.      |
| Winning      | Game Event     | Character has won the match.              |

#### Animation / Timeline

Each state in the state machine can be associated with a timeline which can
display character or character accessory renderables (see the
*Visuals* section), play sounds (see *Audio* section) or generate events for
the game to react to (see `EventId` in
`src/flatbufferschemas/pie_noon_common.fbs`).  Each item on the timeline is
associated with a timestamp (in milliseconds) which indicates when the
item happen relative to the start of the state.  When the state machine enters
a state it starts playing through items in the timeline.  The state machine
will generate the `AnimationEnd` event when the timeline's `end_time` has
been reached.

For example, the following state will generate the `AnimationEnd` event one
second after entering the state and will attempt to transition to the
`Throwing` state:

    {
      "states": [
        {
          "id": "Loading_1",
          "transitions": [
            {
              "target_state": "Throwing",
              "condition": {
                "logical_inputs": "AnimationEnd"
              }
            },
          ],
          "timeline": {
            "end_time": 1000,
          }
        }
      ]
    }

The following state will display `CharacterLoad1` when entering the state
and `CharacterLoad2` 0.5 seconds after the timeline started playback:

    {
      "states": [
        {
          "id": "Loading_1",
          "transitions": [
            {
              "target_state": "Throwing",
              "condition": {
                "logical_inputs": "AnimationEnd"
              }
            },
          ],
          "timeline": {
            "end_time": 1000,
            "renderables": [
              {
                "time": 0,
                "renderable": "RenderableId.CharacterLoad1"
              },
              {
                "time": 500,
                "renderable": "RenderableId.CharacterLoad2"
              }
            ]
          }
        }
      ]
    }

<br>

  [cwebp]: https://developers.google.com/speed/webp/docs/cwebp
  [Flatbuffers]: http://google.github.io/flatbuffers/
  [Flatbuffers compiler]: http://google.github.io/flatbuffers/md__compiler.html
  [Flatbuffers schema]: http://google.github.io/flatbuffers/md__schemas.html
  [JSON]: http://json.org/
  [Python]: http://python.org/
  [webp]: https://developers.google.com/speed/webp/
  [Windows]: http://windows.microsoft.com/
