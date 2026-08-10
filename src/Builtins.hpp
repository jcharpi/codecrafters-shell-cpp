#pragma once

#include <algorithm>
#include <cstdlib> // getenv, exit
#include <filesystem>
#include <functional>
#include <iostream>
#include <print>
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
    println(cerr, "cd: {}: No such file or directory", path);
  }
}

inline void handle_echo(const vector<string>& args) {
  for (ptrdiff_t i = 1; i < ssize(args); i++) {
    print(cout, "{}{}", i > 1 ? " " : "", args[i]);
  }
  println(cout);
}

inline void handle_exit(const vector<string>&) { exit(EXIT_SUCCESS); }

inline void handle_pwd(const vector<string>&) { println(cout, "{}", filesystem::current_path().string()); }

inline void handle_type(const vector<string>& args) {
  const string& target = ssize(args) > 1 ? args[1] : "";

  if (ranges::contains(builtin_names, target)) {
    println(cout, "{} is a shell builtin", target);
  } else if (string file_path = executable_in_path(target); !file_path.empty()) {
    println(cout, "{} is {}", target, file_path);
  } else {
    println(cout, "{}: not found", target);
  }
}

// args[0] is "complete"; args[1] is the option. The operands after it depend on
// the option:
//   -C <script> <command>   register <script> as the completer for <command>
//   -r <command>            remove <command>'s completer
//   -p <command>            print <command>'s completer as a `complete -C` line
inline void handle_complete(const vector<string>& args) {
  if (ssize(args) < 2) return;
  const string& option = args[1];

  if (option == "-C" && ssize(args) >= 4) {
    const string& script = args[2];
    const string& command = args[3];
    registered_completer_scripts[command] = script;
    return;
  }

  if (option == "-r" && ssize(args) >= 3) {
    const string& command = args[2];
    registered_completer_scripts.erase(command);
    return;
  }

  if (option == "-p" && ssize(args) >= 3) {
    const string& command = args[2];
    if (const string* path = registered_completer_for(command)) {
      println(cout, "complete -C '{}' {}", *path, command);
    } else {
      println(cerr, "complete: {}: no completion specification", command);
    }
  }
}

using BuiltinHandler = function<void(const vector<string>&)>;

inline const unordered_map<string, BuiltinHandler> builtins{{"cd", handle_cd},     {"echo", handle_echo},
                                                            {"exit", handle_exit}, {"pwd", handle_pwd},
                                                            {"type", handle_type}, {"complete", handle_complete}};
