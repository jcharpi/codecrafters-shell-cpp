#pragma once

#include <string>
#include <unordered_map>

using namespace std;

// Maps a command to the completer script registered for it via `complete -C`.
inline unordered_map<string, string> registered_completer_scripts;

// Returns the script registered for `command`, or nullptr when there is none.
inline const string* registered_completer_for(const string& command) {
  auto registration = registered_completer_scripts.find(command);
  if (registration == registered_completer_scripts.end()) {
    return nullptr;
  }
  return &registration->second;
}
