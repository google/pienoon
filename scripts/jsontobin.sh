#!/bin/bash

# A script to generate the bin files from JSON files, which splat uses for
# loading certain game assets
./flatbuffers/flatc \
  -o assets/ \
  -b src/flatbufferschemas/character_state_machine_def.fbs \
  src/character_state_machine_def.json

for filename in src/materials/*.json; do
  ./flatbuffers/flatc -o assets/materials/ \
    -b src/flatbufferschemas/materials.fbs \
    "$filename"
done

