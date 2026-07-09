#pragma once

#include <readline/readline.h>

#include <cstdlib>
#include <cstring>
#include <readline/readline.h>
#include <string>
#include <string_view>
#include <vector>

#include "Builtins.hpp"
#include "Executables.hpp"

inline char *next_matching_candidate(const char *text, int state,
                                     const std::vector<std::string> &candidates,
                                     size_t &cursor) {
  if (state == 0) {
    cursor = 0;
  }

  while (cursor < candidates.size()) {
    const std::string &candidate = candidates[cursor];
    cursor++;
    if (candidate.starts_with(text)) {
      return strdup(candidate.c_str());
    }
  }

  return nullptr;
}

inline char *complete_builtin_generator(const char *text, int state) {
  static size_t cursor;
  return next_matching_candidate(text, state, builtin_names, cursor);
}

inline char *complete_executable_generator(const char *text, int state) {
  static std::vector<std::string> executables;
  static size_t cursor;

  if (state == 0) {
    executables = executable_names_in_path();
  }

  return next_matching_candidate(text, state, executables, cursor);
}

inline char *complete_entry_generator(const char *text, int state) {
  static std::vector<std::string> files;
  static size_t cursor;

  if (state == 0) {
    files = entries_in_directory(text);
  }

  return next_matching_candidate(text, state, files, cursor);
}

inline char **try_completion(const char *text, int start, int /*end*/) {
  rl_attempted_completion_over = 1;

  if (start == 0) {
    char **builtin_matches =
        rl_completion_matches(text, complete_builtin_generator);
    if (builtin_matches) {
      return builtin_matches;
    }

    return rl_completion_matches(text, complete_executable_generator);
  }

  char **entry_matches = rl_completion_matches(text, complete_entry_generator);
  // If exactly one match and that match is a directory, don't append space to
  // the end of completion
  if (entry_matches && !entry_matches[1] &&
      std::string_view(entry_matches[0]).ends_with('/')) {
    rl_completion_append_character = '\0';
  }
  return entry_matches;
}

inline void setup_completion() {
  rl_attempted_completion_function = try_completion;
}
