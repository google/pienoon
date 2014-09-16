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

#include "precompiled.h"

#include "utilities.h"

namespace fpl {

bool LoadFile(const char *filename, std::string *dest) {
  auto handle = SDL_RWFromFile(filename, "rb");
  if (!handle) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "LoadFile fail on %s", filename);
    return false;
  }
  auto len = static_cast<size_t>(SDL_RWseek(handle, 0, RW_SEEK_END));
  SDL_RWseek(handle, 0, RW_SEEK_SET);
  dest->assign(len + 1, 0);
  size_t rlen = static_cast<size_t>(SDL_RWread(handle, &(*dest)[0], 1, len));
  SDL_RWclose(handle);
  return len == rlen && len > 0;
}

#if defined(_WIN32)
inline char* getcwd(char *buffer, int maxlen) {
  return _getcwd(buffer, maxlen);
}

inline int chdir(const char *dirname) {
  return _chdir(dirname);
}
#endif  // defined(_WIN32)

// If target_dir exists under the current directory, change into it.
// Otherwise, check each path in suffix_dirs. When one matches the end of the
// current path, pop it off the current path. Then change into target_dir.
//
// This function is useful when you have some idea what your current directory
// will be, but there are several possibilities. For example, you might be in
// the "Debug" or "Release" directory, and need to transition to the "assets"
// directory. You could specify target_dir as "assets", and suffic_dirs as
// {"Debug", "Release"}.
bool ChangeToUpstreamDir(const char* const target_dir,
                         const char* const suffix_dirs[],
                         size_t num_suffixes) {
#if !defined(__ANDROID__)
  {
    char path[256];
    char *dir = getcwd(path, sizeof(path));
    assert(dir);

    // Return if we're already in the assets directory.
    const char* last_slash = strrchr(path, flatbuffers::kPathSeparator);
    const char* last_dir = last_slash ? last_slash + 1 : path;
    if (strcmp(last_dir, target_dir) == 0)
      return true;

    // Change to the directory one above the assets directory.
    const size_t len_dir = strlen(dir);
    int success;
    for (size_t i = 0; i < num_suffixes; ++i) {
      const size_t len_build_path = strlen(suffix_dirs[i]);
      char* beyond_base = &dir[len_dir - len_build_path];
      if (strcmp(beyond_base, suffix_dirs[i]) != 0)
        continue;

      *beyond_base = '\0';
      success = chdir(dir);
      assert(success == 0);
      break;
    }

    // Change into assets directory.
    success = chdir(target_dir);
    if (success != 0) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Unable to change into %s dir\n", target_dir);
      return false;
    }
  }
#endif // !defined(__ANDROID__)
  return true;
}

static inline bool IsUpperCase(const char c) {
  return c == toupper(c);
}

std::string CamelCaseToSnakeCase(const char* const camel) {
  // Replace capitals with underbar + lowercase.
  std::string snake;
  for (const char* c = camel; *c != '\0'; ++c) {
    if (IsUpperCase(*c)) {
      const bool is_start_or_end = c == camel || *(c + 1) == '\0';
      if (!is_start_or_end) {
        snake += '_';
      }
      snake += static_cast<char>(tolower(*c));
    } else {
      snake += *c;
    }
  }
  return snake;
}

std::string FileNameFromEnumName(const char* const enum_name,
                                 const char* const prefix,
                                 const char* const suffix) {
  // Skip over the initial 'k', if it exists.
  const bool starts_with_k = enum_name[0] == 'k' && IsUpperCase(enum_name[1]);
  const char* const camel_case_name = starts_with_k ? enum_name + 1 : enum_name;

  // Assemble file name.
  return std::string(prefix)
       + CamelCaseToSnakeCase(camel_case_name)
       + std::string(suffix);
}

} // namespace fpl
