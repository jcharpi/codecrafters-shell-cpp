#pragma once

#include <readline/readline.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "ArgSplitter.hpp"
#include "Builtins.hpp"
#include "CompleterScript.hpp"
#include "CompletionRegistry.hpp"
#include "Executables.hpp"

using namespace std;

inline char* next_matching_candidate(const char* text, int state, const vector<string>& candidates, ptrdiff_t& cursor) {
  if (state == 0) {
    cursor = 0;
  }

  while (cursor < ssize(candidates)) {
    const string& candidate = candidates[cursor];
    cursor++;
    if (candidate.starts_with(text)) {
      return strdup(candidate.c_str());
    }
  }

  return nullptr;
}

inline char* complete_builtin_generator(const char* text, int state) {
  static ptrdiff_t cursor;
  return next_matching_candidate(text, state, builtin_names, cursor);
}

inline char* complete_executable_generator(const char* text, int state) {
  static vector<string> executables;
  static ptrdiff_t cursor;

  if (state == 0) {
    executables = executable_names_in_path();
  }

  return next_matching_candidate(text, state, executables, cursor);
}

inline char* complete_entry_generator(const char* text, int state) {
  static vector<string> files;
  static ptrdiff_t cursor;

  if (state == 0) {
    files = entries_in_directory(text);
  }

  return next_matching_candidate(text, state, files, cursor);
}

// Filled by try_completion before it hands the generator to readline, since
// readline generators take no user data of their own.
inline vector<string> completer_candidates;

inline char* complete_registered_generator(const char* text, int state) {
  static ptrdiff_t cursor;
  return next_matching_candidate(text, state, completer_candidates, cursor);
}

inline char** try_completion(const char* text, int start, int /*end*/) {
  rl_attempted_completion_over = 1;

  if (start == 0) {
    char** builtin_matches = rl_completion_matches(text, complete_builtin_generator);
    if (builtin_matches) {
      return builtin_matches;
    }

    return rl_completion_matches(text, complete_executable_generator);
  }

  vector<string> line_words = split_args(string(rl_line_buffer).substr(0, start));
  if (!line_words.empty()) {
    if (const string* script = registered_completer_for(line_words.front())) {
      const string previous_word = ssize(line_words) > 1 ? line_words.back() : "";
      completer_candidates = run_completer_script(*script, line_words.front(), text, line_words.back());
      return rl_completion_matches(text, complete_registered_generator);
    }
  }

  char** entry_matches = rl_completion_matches(text, complete_entry_generator);
  // If exactly one match and that match is a directory, don't append space to
  // the end of completion
  if (entry_matches && !entry_matches[1] && string_view(entry_matches[0]).ends_with('/')) {
    rl_completion_append_character = '\0';
  }
  return entry_matches;
}

inline void setup_completion() { rl_attempted_completion_function = try_completion; }
