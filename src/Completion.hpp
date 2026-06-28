#pragma once

#include <readline/readline.h>

#include <cstdlib>
#include <cstring>
#include <readline/readline.h>
#include <string>
#include <vector>

#include "Builtins.hpp"
#include "Executables.hpp"

inline char *complete_builtin_generator(const char *text, int state) {
  static size_t cursor;
  if (state == 0) {
    cursor = 0;
  }

  while (cursor < builtin_names.size()) {
    const std::string &builtin = builtin_names[cursor];
    cursor++;
    if (builtin.starts_with(text)) {
      return strdup(builtin.c_str());
    }
  }

  return nullptr;
}

inline char *complete_executable_generator(const char *text, int state) {
  static std::vector<std::string> executables;
  static size_t cursor;
  if (state == 0) {
    executables = executable_names_in_path();
    cursor = 0;
  }

  while (cursor < executables.size()) {
    const std::string &executable_name = executables[cursor];
    cursor++;
    if (executable_name.starts_with(text)) {
      return strdup(executable_name.c_str());
    }
  }

  return nullptr;
}

inline char *complete_file_generator(const char *text, int state) {
  static std::vector<std::string> files;
  static size_t cursor;
  
  if (state == 0) {
    files = file_names_in_cwd();
    cursor = 0;
  }

  while (cursor < files.size()) {
    const std::string &file_name = files[cursor];
    cursor++;
    if (file_name.starts_with(text)) {
      return strdup(file_name.c_str());
    }
  }

  return nullptr;
}

inline char **try_completion(const char *text, int /*start*/, int /*end*/) {
  rl_attempted_completion_over = 1;
  char **builtin_matches =
      rl_completion_matches(text, complete_builtin_generator);
  if (builtin_matches) {
    return builtin_matches;
  }

  char **executable_matches = rl_completion_matches(text, complete_executable_generator);
  if (executable_matches) {
    return executable_matches;
  }

  return rl_completion_matches(text, complete_file_generator);
}

inline void setup_completion() {
  rl_attempted_completion_function = try_completion;
}
