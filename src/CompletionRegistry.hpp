#pragma once

#include <string>
#include <unordered_map>

// Maps a command to the completer script registered for it via `complete -C`.
inline std::unordered_map<std::string, std::string> registered_completer_scripts;

// Returns the script registered for `command`, or nullptr when there is none.
inline const std::string *registered_completer_for(const std::string &command) {
  auto registration = registered_completer_scripts.find(command);
  if (registration == registered_completer_scripts.end()) {
    return nullptr;
  }
  return &registration->second;
}
