#!/bin/bash -eu

declare -r script_dir="$(cd $(dirname $0); pwd)"
declare -r extension=$(uname -a | grep Cygwin && echo -n .exe || echo -n)
declare -r flatc=flatc${extension}
declare -r flatbuffers_build_dir=${script_dir}/../flatbuffers

: ${FLATBUFFERS_SEARCH_PATHS:=\
    "${flatbuffers_build_dir}/${flatc} \
     ${flatbuffers_build_dir}/Release/${flatc} \
     ${flatbuffers_build_dir}/Debug/${flatc}"}

# Search PATH and FLATBUFFERS_SEARCH_PATHS for flatc writing the path to the
# executable that is found to the standard output.  If flatc isn't found an
# error is written to stderr and this function returns 1.
find_flatc() {
  local found_exe="$(which ${flatc})"
  if [[ ! -e "${found_exe}"  ]]; then
    for path in ${FLATBUFFERS_SEARCH_PATHS}; do
      if [[ -e "${path}" ]]; then
	    found_exe="${path}"
		break
      fi
	done
  fi
  if [[ ! -e "${found_exe}" ]]; then
	echo "${flatc} not found" >&2
	return 1
  fi
  echo "${found_exe}"
  return 0
}

flatc_exe=$(find_flatc)
[[ ! -e "${flatc_exe}" ]] && exit 1

# Create the output directories.
mkdir -p assets/materials

# A script to generate the bin files from JSON files, which splat uses for
# loading certain game assets
${flatc_exe} \
  -o assets/ \
  -b src/flatbufferschemas/character_state_machine_def.fbs \
  src/character_state_machine_def.json

for filename in src/materials/*.json; do
  ${flatc_exe} -o assets/materials/ \
    -b src/flatbufferschemas/materials.fbs \
    "$filename"
done

