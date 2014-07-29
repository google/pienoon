#!/bin/bash

# A script to generate the bin files from JSON files, which splat uses for
# loading certain game assets
./flatbuffers/flatc \
  -o assets/ \
  -b src/character_state_machine_def.fbs \
  src/character_state_machine_def.json
