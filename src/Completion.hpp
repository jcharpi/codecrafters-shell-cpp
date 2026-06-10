#pragma once

#include <readline/readline.h>

#include <cstdlib>
#include <cstring>
#include <readline/readline.h>
#include <string>
#include <vector>

#include "Builtins.hpp"

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

inline char **try_completion(const char *text, int /*start*/, int /*end*/) {
  rl_attempted_completion_over = 1;
  return rl_completion_matches(text, complete_builtin_generator);
}

inline void setup_completion() {
  rl_attempted_completion_function = try_completion;
}
