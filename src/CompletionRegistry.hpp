#pragma once
#include <string>
#include <unordered_map>

inline std::unordered_map<std::string, std::string> registered_completer_scripts;

inline const std::string *registered_completer_for(const std::string &command) {
    auto registration = registered_completer_scripts.find(command);
    if (registration == registered_completer_scripts.end()) return nullptr;

    return &registration->second;
}