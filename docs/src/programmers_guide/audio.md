Audio {#pie_noon_guide_audio}
=====

# Overview {#pie_noon_guide_audio_overview}

The audio engine manages playing both sounds and music.  It is largely
data-driven.  In a config file, settings such as the number of output channels
and the buffer size can be adjusted.  The audio engine will manage these
channels, ensuring that if this limit is exceeded only the lowest priority
sounds are dropped.

# Sound Collections {#pie_noon_guide_audio_sound_collections}

Individual sounds are represented by `SoundSource` objects.  A `SoundSource` is
not played directly though.  `SoundSource`s are gathered into a class called
`SoundCollection` which represents a collection of related sounds.  For
example, in Pie Noon there are many different sounds for a pie hitting a
character to avoid the monotony of every hit sounding the same.

Each `SoundCollection` has a unique integer identifier.  To play a given
`SoundCollection`, its identifier is passed to `AudioEngine::PlaySound`.

# Buses {#pie_noon_guide_audio_buses}

The main purpose of a bus is to adjust the gain (or volume) of a group of
sounds in tandem.  Any number of buses may be defined.

All `SoundCollection`s define which bus they are to be played on.  Examples
buses might be 'music', 'ambient_sound', 'sound_effects'.  A bus may list other
buses as 'child_buses', which means that all changes to that bus's gain level
affect all children of that bus as well.  There is always a single 'master'
bus, which is the root bus which all other buses decend from.  A bus may also
define 'duck_buses', which are buses that will lower in volume when a sound is
played on that bus.  For example, a designer might want to have background
sound effects and music lower in volume when an important dialog is playing.
To do this the sound effect and music buses would be added to the 'duck_buses'
list in the dialog bus.

# Configuration {#pie_noon_guide_audio_configuration}

The following configuration files control the behavior of the audio engine.

| JSON File(s)                 | Flatbuffers Schema                            |
|------------------------------|-----------------------------------------------|
| `config.json`                | `audio_config.fbs` (included in `config.fbs`) |
| `buses.json`                 | `buses.fbs`                                   |
| `sound_assets.json`          | `sound_assets.fbs`                            |
| `sounds/*.json`              | `sound_collection_def.fbs`                    |

* `audio_config.fbs`

  The main configuration schema for the audio engine is defined in
  audio_config.fbs and it is populated in config.json.  This controls the
  settings of the audio engine itself, like the number of output channels and
  the sample rate.

* `buses.fbs`

  The list of all bus definitions is defined in buses.fbs and populated in
  buses.json.

* `sound_collection_def.fbs`

  The `SoundCollection` schema is defined in `sound_collection_def.fbs.`
  Individual `SoundCollectionDef`s are defined in the `sounds/` subdirectory,
  one SoundCollectionDef per file.

* `sound_assets.fbs`

  Another file, sounds_assets.json, is used to load these individual
  `SoundCollection`s.  Filenames listed in this JSON file will be loaded in
  order.  The index of a sound collection in this file represents the unique
  identifier used to play a that sound collection.   E.g., the first sound
  sound collection listed would be played by calling
  `audio_engine.PlaySound(0)`.
