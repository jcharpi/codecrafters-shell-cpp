#pragma once

#include <algorithm>
#include <cstdlib> // getenv, exit
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "CompletionRegistry.hpp"
#include "Executables.hpp"

using namespace std;

inline const vector<string> builtin_names{"cd", "echo", "exit", "pwd", "type", "complete"};

inline void handle_cd(const vector<string>& args) {
  if (ssize(args) < 2) return;
  const string& path = args[1];
  if (path == "~") {
    const char* home = getenv("HOME");
    if (home) {
      filesystem::current_path(home);
    }
  } else if (filesystem::is_directory(path)) {
    filesystem::current_path(path);
  } else {
    cerr << "cd: " << path << ": No such file or directory\n";
  }
}

inline void handle_echo(const vector<string>& args) {
  for (ptrdiff_t i = 1; i < ssize(args); i++) {
    cout << (i > 1 ? " " : "") << args[i];
  }
  cout << "\n";
}

inline void handle_exit(const vector<string>&) { exit(EXIT_SUCCESS); }

inline void handle_pwd(const vector<string>&) { cout << filesystem::current_path().string() << "\n"; }

inline void handle_type(const vector<string>& args) {
  const string& target = ssize(args) > 1 ? args[1] : "";

  if (ranges::contains(builtin_names, target)) {
    cout << target << " is a shell builtin\n";
  } else if (string file_path = executable_in_path(target); !file_path.empty()) {
    cout << target << " is " << file_path << "\n";
  } else {
    cout << target << ": not found\n";
  }
}

inline void handle_complete(const vector<string>& args) {
  if (ssize(args) >= 4 && args[1] == "-C") {
    registered_completer_scripts[args[3]] = args[2];
    return;
  }

  if (ssize(args) >= 3 && args[1] == "-p") {
    const string& command = args[2];
    if (const string* path = registered_completer_for(command)) {
      cout << "complete -C '" << *path << "' " << command << "\n";
    } else {
      cerr << "complete: " << command << ": no completion specification\n";
    }
  }
}

using BuiltinHandler = function<void(const vector<string>&)>;

inline const unordered_map<string, BuiltinHandler> builtins{{"cd", handle_cd},     {"echo", handle_echo},
                                                            {"exit", handle_exit}, {"pwd", handle_pwd},
                                                            {"type", handle_type}, {"complete", handle_complete}};
