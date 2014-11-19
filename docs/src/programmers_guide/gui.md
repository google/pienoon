Pie Noon GUI System {#pie_noon_guide_gui}
===================

# Introduction {#gui_introduction}

This document outlines the GUI components used to construct the menus in Pie
Noon.

# High Level Goals   {#gui_high_level_goals}

The GUI menu system is designed to provide an easy way to author data-driven
menus that automatically handle input from both touch devices and gamepads.

# Organization {#gui_organization}

Basic input is provided by the Input system (`input.h/cpp`).  This provides
access to raw input data such as key presses, joystick/gamepad movements, and
mouse/pointer touch events.

The system is built upon the `TouchscreenButton` class,
(`touchscreenbutton.cpp/h`) which represents an onscreen button which responds
to touch events and can easily be queried to learn if it has been pushed
recently.  Each item in the menu is a `TouchscreenButton`.

`TouchscreenButtons` can be used independently, but most collections of buttons
in the game are written using `GuiMenus`, (`gui_menu.cpp/.h`) which are
organizations of `TouchscreenButtons`, along with whatever static images are
necessary to render the menu on screen.

The `GuiMenu` class takes care of updating and maintaining the
`TouchScreenButtons`, as well as tracking the menu focus and handling movement
and selection from gamepads/keyboards and other non-touch sources.

# TouchscreenButtons {#gui_touchscreenbuttons}

Each `TouchscreenButton` represents a single button on-screen.  As long as
something calls its `AdvanceFrame()` member function every frame, it will keep
itself up to date, and track touch events.  It can be queried via the
`button()` member function, which returns an input button object, (a standard
object in the [input][] library, for tracking input states via `is_down()`,
`went_down()`, and `went_up()`.) This is used to check both the current state
of the button, as well as any changes that have taken place since the last
update.

All of the details for each button are read in from a [Flatbuffer][] data
object, `button_def`.  (Located in `common.fbs/json`)  This describes both the
look of the button, (textures and offests to use when rendering the button when
up/down), the screen-area that the button registers touches in, and a unique
button ID.

# GuiMenu {#gui_guimenu}

The `GuiMenu` object represents a menu that the player can navigate using their
keyboard or a gamepad.  It maintains a list of active buttons that are part of
the menu, handles transitions between them, and registers events if a menu item
is selected via gamepad, or clicked with a touch device.

Any time a button is pressed the `GuiMenu` records this as an event, and stores
it in a queue.  The game can request the oldest unhandled menu selection event
via the `GetRecentSelection()` funtion, and `handle` them accordingly.  (Events
are returned as a structure containing the ID of the button that was pressed,
and the ID of the controller that completed the action.)

Much like `TouchscreenButton`s, `GuiMenus` are described by a [Flatbuffer][]
data object, which lists all of the buttons in the current menu, as well as
things like whether the buttons should respond to controller input or be touch
only or which button should start out selected.

The game maintains one `GuiObject`, which simply loads up different menus when
called for.

# StaticImage {#gui_staticimage}

`GuiMenus` also contain static images - other art necessary for rendering the
menu, but that is not interactive.  Like other parts of the menu, static images
are also loaded from the [Flatbuffer][] configuration file.  Structurally, they
are effectively buttons, stripped of most of their interactivity.

# Menu Button States {#gui_menu_button_states}

The `GuiMenu` allows for several possible states the buttons can be in at any
given time.  These states are tracked on the buttons, but largely only make
sense in the context of the menu itself.

## Focus {#gui_menu_focus}

The focus is the button that is currently "selected" in the menu.  It a gamepad
user were to push the A button, whatever button currently has focus would
trigger.  Only one button can have focus at a time, and this is tracked by the
menu itself.  The game can check the current focus via the `GetFocus()` method.

When a button has focus, it will automatically animate and pulse.

## Disabled {#gui_menu_disabled}

When buttons are set to be disabled, they are greyed out, and cannot be
selected.  They can still receive focus, but if the user tries to select them,
instead of a normal event being sent, they send an event of type
`kInvalidInput`.

## Visible {#gui_menu_visible}

Buttons are normally visible, but this can be changed.  If a button's
visibility is set to false, then the button does not render on screen, and
cannot receive focus or be selected.


  [FlatBuffer]: http://google.github.io/flatbuffers/
  [input]: @ref engine_input
