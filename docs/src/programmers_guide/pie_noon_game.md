PieNoonGame    {#pie_noon_guide_pie_noon_game}
===========

The high-level class that holds the state and controls the flow of the game
is called `PieNoonGame`. Find the source in `src/pie_noon_game.cpp`.

## Initialization

All game systems are initialized in `PieNoonGame::Initialize()`. Initializations
that require a non-trivial amount of time to complete are performed on a
worker thread in the background. Presently, these non-trivial initializations
are limited to the loading of rendering assets.

## Main Loop

`PieNoonGame::Run` has the main game loop. One iteration of the `while` loop in
that function is one frame of the game update.

The frame length varies with the speed of the processor. It is clamped, however,
between `config.min_update_time()`, currently 10ms, and
`config.max_update_time()`, currently 100ms. To change these values, see
[Modifying Assets][], and in particular, `config.json`.

When the game is playing, the game loop can be summarize as follows,
  - the game simulation is updated with GameState::AdvanceFrame().
  - the SceneDescription--a description of the poses and positions of the
    characters, camera, and environment--is captured with
    GameState::PopulateScene().
  - the SceneDescription is rendered with PieNoonGame::Render().
  - the GUI is rendered with PieNoonGame::Render2DElements().
Note that the game simulation and rendering system are isolated from each other.
The only communication between the simulation and rendering occurs via
SceneDescription.

## State Machine

`PieNoonGame` has one high-level state variable that dictates the UI flow.
The states and their transitions are described below.

  * `kLoadingInitialMaterials` -- The rendering materials required to display
    the loading screen itself are being loaded. Nothing is displayed on the
    screenat this time.

    When the initial materials have been loaded, the game transitions to
    kLoaded.

  * `kLoading` -- The rendering materials and audio assets for the game are
    being loaded. Note that loading occurs on a worker thread via AsyncLoader.
    This allows the loading screen to be rendered by the main thread while
    the worker thread loads the assets in the background.

    When all assets have been loaded, the game transitions to kTutorial if
    the player has never seen the tutorial before, or kFinished if the the
    tutorial has already been seen.

  * `kTutorial` -- A slide show is displayed describing how to play the game.
    The slides are large textures, so to save on memory, we only read a
    couple slides in advance, and unload a slide as soon as it stops being
    displayed.

    After the slide show, we transition to kFinished.

  * `kFinished` -- The menu is display. Users can log in to Google Play Games,
    view achievements and leaderboards, see the license and about texts,
    rewatch the tutorial, or start playing the game.

    The AI characters play the game in the background.

    If the tutorial is selected, the game transitions to kTutorial.

    If "Start!" is selected, the game transitions to kJoining for multi-player
    games, or kPlaying for single player games. A game is single player if it
    is started via the touch screen.

    If the back button is selected, the game exits.

  * `kJoining` -- The game counts down to game start. During the countdown,
    players can join the game by triggering any input on their controller.
    When joining, the player's character will jump, change color, and sparkle.
    This is intended to let the player see which character he or she is playing.

    Once the countdown completes, the game transitions to kPlaying.

  * `kPlaying` -- The game is active.

    Once all the user controlled characters have been eliminated from the game,
    or there is only one or no characters left, the game transitions to the
    kFinished state.

    If the back button (or 'p' on the keyboard) is pressed, or if the game is
    interrupted (e.g. it is minimized), the game transitions to the kPaused
    state.

  * `kPaused` -- The game is paused and the pause menu is displayed.

    If the back button is pressed, the game exits. If 'resume' is selected,
    the game transitions back to kPlaying.


## Configuration

Game constants are kept in configuration files in [JSON][] format.
The configuration files are in the src/rawasset directory.

*  config.json -- majority of game constants
*  character_state_machine_def.json -- game rules and character animations
*  buses.json -- sound bus configuration
*  sound_assets.json -- mapping of sound files

The [JSON][] file formats are defined in corresponding FlatBuffer schemas
(.fbs files). These schemas are in the src/flatbufferschemas directory.

The [JSON][] files are compiled by the asset pipeline into bin files with the
command `make assets`. This command runs `flatc` to convert config.fbs with
config.fbs into config.bin, for instance.

See [Asset Modification][] for further details.

<br>

  [Asset Modification]: @ref pie_noon_guide_assets
  [JSON]: http://json.org/
  [Modifying Assets]: @ref pie_noon_guide_assets
