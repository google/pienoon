Pie Noon Engine

## Introduction

This document outlines the engine components upon which the rest
of Pie Noon is built.

## SDL

We use [SDL] (Simple Directmedia Layer) as our lowest level layer. SDL is an
Open Source cross platform layer providing OpenGL context creation, input, and
other things a game needs to run on a platform, without having to write any
platform specific code. It has been in development for a long time, and has
very robust support for mobile (Android, iOS), desktop (Windows, OS X, Linux),
and even is available for the web through asm.js.

SDL together with OpenGL(ES) and C++ provide an excellent basis for making great
cross platform games.

## Components

Directly on top of SDL sit two systems, the renderer and input systems.
On top of the renderer sits two more (optional) systems, material manager and
the asynchronous loader.

The renderer also depends upon our [MathFu] library for all its vector and
matrix datatypes. The (optional) material manager depends on our [FlatBuffers]
serialization library.

## Renderer

The Renderer (`renderer.h/.cpp`) is the core of the engine, and is responsible
for creating the OpenGL context and OpenGL resources such as
shaders and textures.

To actually represent these resources, we have:
* `Shader` (`shader.h/.cpp`)
* `Mesh` (`mesh.h/.cpp`)
* `Texture` / `Material` (`material.h/.cpp`)

The basic flow of using this lower level layer is as follows:

* Instantiate the `Renderer`, and call `Initialize` on it. This will get your
  OpenGL context set up, and a window/screen ready to draw on.
* Load resources. `CompileAndLinkShader` will turn two shader strings (GLSL
  vertex and pixel shader) into a `Shader` object
  (for even more convenient methods that will load directly from file, see the
  material manager below).
  `LoadAndUnpackTexture` will take a file (currently TGA or WebP formats)
  and turn it into a raw buffer which `CreateTexture` turns into an OpenGL
  texture.
  Methods like this can generate complex errors, so whenever anything fails in
  the renderer, human readable errors are available in last_error().
* Create a `Mesh`. You supply vertex data, and add one or more sets of indices
  referring to it with `AddIndices`.
* Now you're ready to run your main loop. Call `AdvanceFrame` on the renderer
  to swap buffers and do general initialisation of the frame, likely followed
  by `ClearFrameBuffer`.
* Before rendering anything, set up the renderer's `model_view_projection()`.
  Use our separate [MathFu] library to combine matrices depending on whether
  you're creating a 2D or 3D scene, e.g. `mathfu::OrthoHelper` and
  `mathfu::PerspectiveHelper`.
* Now use the shader you've created by calling `Set` on it. This will make it
  active, and also upload any renderer variables (such as
  `model_view_projection()`), ready to be used by the shader.
* Finally, calling `Render` on your mesh will display it on screen.

## Material Manager

The renderer is deliberately a bare minimum system that takes care of creating
and using resources, but not *managing* them. The material manager takes care
of loading from disk, and caching resources, but is deliberately seperate from
the renderer, such that it is easy to replace with something else should the
need arise.

Where the renderer reads from memory buffers, the material manager loads files.
To keep this cross platform, any resources should be in a folder called `assets`
under the project root. All paths you specify are relative to this folder.

Once you've instantiated the `MaterialManager`, calls like `LoadShader`
will conveniently construct a `Shader` from two files. It does this by
suffixing `.glslv` and `.glslf` to the filename you pass.

Similarly, `LoadTexture` will load a TGA or WebP file straight into a `Texture`.

All methods that start with `Load` have the property that if you ask to load a
file that has been loaded before, it just instantly return the previously
created resource, and only do any actual loading if not.

Alternatively, there are `Find` versions of these methods that will return
`nullptr` if the resource wasn't previously loaded.

More high-level than loading individual textures is loading a `Material`,
which is a set of textures all meant to be used in the same draw call,
bundled with rendering flags such as the desired alpha blending mode etc.
You create an actual material file by writing a small .json file which specifies
the texture filenames and other properties. This JSON file can be converted
to a binary file by our [FlatBuffers] serialization library, the result
of which can be passed to `LoadMaterial` that will load all referenced
textures and create a `Material` object (which can be attached to `Mesh`).

## Asynchronous Loader

Loading resources can take a long time, and can be sped up by loading
heavy resources (such as textures, WebP in particular) in parallel with the
rest of the game initialisation. Even if speed is not the issue, just being
able to conveniently render a loading animation is nice to have.

`AsyncLoader` takes care of all that, hiding the gory details of threading
from you. It is fully integrated with the material manager, so using it is
relatively simple:

* Load all your resources as normal. You will receive `Texture` and
  `Material` objects, but these won't actually have any texture data
  associated with them yet, as textures haven't loaded yet.
* Once you've queued up all your resources, call `StartLoadingTextures`
  on the material manager to get the loading started.
* Now, enter your frame loop as normal. Call `TryFinalize` which will check
  if all textures have been loaded. If it returns false, you should display
  a loading screen, otherwise render the game as normal.
* Your loading screen might want to use textures also. Since the loader
  loads textures in the order they were requested, make sure you queue up
  your loading screen textures first. If the `Texture::id()` is non-zero,
  it can already be used.

## Input System

`InputSystem` (`input.h/.cpp`) deals with time, touch/mouse/keyboard/gamepad
input, and lifecycle events.

Similar to the renderer, instantiate it, then call `Initialize`, and once
per frame call `AdvanceFrame` right after you called the same method on
the renderer. This collects new input events from the system to be reflected
in its internal state.

Call `Time` for seconds since game start, and `DeltaTime` for seconds since
the last frame.

Now you can query the state of the inputs you're interested in. For example,
call `GetButton` with one of the constants from `input.h` which can either be
a keyboard key, or a pointer (meaning the mouse or touch inputs, depending
on platform) or a gamepad button, and you can see if it went down or up
this last frame, or wether it is currently down. The `Pointer` objects can
tell you more about their current position.



  [SDL]: https://www.libsdl.org/
  [MathFu]: http://google.github.io/mathfu
  [FlatBuffers]: http://google.github.io/flatbuffers/
