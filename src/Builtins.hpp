#pragma once

#include <algorithm>
#include <cstdlib> // std::getenv, std::exit
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Executables.hpp"

inline const std::vector<std::string> builtin_names{
    "cd", "echo", "exit", "pwd", "type",
};

inline void handle_cd(const std::vector<std::string> &args) {
  if (args.size() < 2)
    return;
  const std::string &path = args[1];
  if (path == "~") {
    const char *home = std::getenv("HOME");
    if (home) {
      std::filesystem::current_path(home);
    }
  } else if (std::filesystem::is_directory(path)) {
    std::filesystem::current_path(path);
  } else {
    std::cerr << "cd: " << path << ": No such file or directory\n";
  }
}

inline void handle_echo(const std::vector<std::string> &args) {
  for (size_t i = 1; i < args.size(); i++) {
    std::cout << (i > 1 ? " " : "") << args[i];
  }
  std::cout << "\n";
}

inline void handle_exit(const std::vector<std::string> &) { std::exit(EXIT_SUCCESS); }

inline void handle_pwd(const std::vector<std::string> &) {
  std::cout << std::filesystem::current_path().string() << "\n";
}

inline void handle_type(const std::vector<std::string> &args) {
  const std::string &target = args.size() > 1 ? args[1] : "";

  if (std::ranges::contains(builtin_names, target)) {
    std::cout << target << " is a shell builtin\n";
  } else if (std::string file_path = executable_in_path(target); !file_path.empty()) {
    std::cout << target << " is " << file_path << "\n";
  } else {
    std::cout << target << ": not found\n";
  }
}

using BuiltinHandler = std::function<void(const std::vector<std::string> &)>;

inline const std::unordered_map<std::string, BuiltinHandler> builtins{
    {"cd", handle_cd}, {"echo", handle_echo}, {"exit", handle_exit}, {"pwd", handle_pwd}, {"type", handle_type},
};
