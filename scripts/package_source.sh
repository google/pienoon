#!/bin/bash -eu
#
# Archive the project directory source along with its' dependencies.

declare -r script_dir="$(cd $(dirname ${0}); pwd)"
declare -r project_dir="$(cd ${script_dir}/..; pwd)"

declare -r external_libs="sdl sdl_mixer vectorial"
declare -r prebuilt_libs="gpg-cpp-sdk"
declare -r google_libs="flatbuffers fplutil googletest mathfu"

declare -r external_libs_path="../../../../external"
declare -r prebuilt_libs_path="../../../../prebuilts"
declare -r google_libs_path="../../libs"

declare -r dependencies="\
  $(for lib in ${external_libs}; do echo ${external_libs_path}/${lib}; done) \
  $(for lib in ${prebuilt_libs}; do echo ${prebuilt_libs_path}/${lib}; done) \
  $(for lib in ${google_libs}; do echo ${google_libs_path}/${lib}; done)"

declare -r temp_dir=$(mktemp -d /tmp/XXXXXX)
declare -r output_dir=${temp_dir}/pie_noon

declare -r nothave_prefixes="\
??
!!"

function clean() {
  rm -rf ${temp_dir}
}

# Clean everything, ignore the ignore rules.
function git_clean_all() {
  pushd "${1}" >/dev/null
  git status -s --ignored | \
    grep -F "${nothave_prefixes}" | \
    awk '{ print $2 }' | \
    xargs rm -rf
  popd >/dev/null
}

# Clean up the temporary directory on exit.
trap clean EXIT

pushd ${project_dir} >/dev/null

# Make sure the source directory is clean.
git_clean_all ${project_dir}
# Copy the project directory.
cp -a ${project_dir} ${output_dir}
rm -rf ${output_dir}/.git

# Create the directory which will contain dependencies.
mkdir -p ${output_dir}/dependencies

# Copy the dependencies
for lib in ${dependencies}; do
  git_clean_all ${lib}
  cp -a ${lib} ${output_dir}/dependencies/
done

find ${output_dir} -type d -name '.git' | xargs rm -rf

# Archive the output directory.
pushd "${temp_dir}" 2>/dev/null
tar -czvf ${project_dir}/pie_noon-src-$(date +%Y%m%d).tar.gz \
  "$(basename ${output_dir})"
