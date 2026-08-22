#pragma once

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <print>
#include <readline/history.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "CompletionRegistry.hpp"
#include "Executables.hpp"
#include "Jobs.hpp"

using namespace std;

// Kept alphabetical, as are the handlers below and the `builtins` map at the
// bottom. Nothing enforces the three agree, so a new builtin means three edits.
inline const vector<string> builtin_names{"cd", "complete", "echo", "exit", "jobs", "pwd", "type", "history"};

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

inline void handle_echo(const vector<string>& args) {
  for (ptrdiff_t i = 1; i < ssize(args); i++) {
    print(cout, "{}{}", i > 1 ? " " : "", args[i]);
  }
  println(cout);
}

inline void handle_exit(const vector<string>&) { exit(EXIT_SUCCESS); }

// Lists the background jobs in job-number order. The status sits
// in a 24-character field whose padding is what separates it from the command:
//   [1]-  Running                 sleep 10
//   [2]+  Running                 sleep 20
inline void handle_jobs(const vector<string>&) {
  reap_background_jobs();
  for (ptrdiff_t index : background_job_display_order()) {
    print_job_line(index);
  }
  purge_done_jobs();
}

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

inline void handle_history(const vector<string>& args) {
  const string& option = ssize(args) > 1 ? args[1] : "";
  if (ssize(args) >= 3) {
    const string& path = args[2];
    if (option == "-r") {
      read_history(path.c_str());
      return;
    }
    if (option == "-w") {
      write_history(path.c_str());
      return;
    }
  }

  int start = history_base;
  if (!option.empty()) {
    int count = 0;
    const char* last = option.c_str() + ssize(option);

    // parse arg to int
    auto [end, error] = from_chars(option.c_str(), last, count);
    if (error != errc{} || end != last || count < 0) {
      println(cerr, "history: {}: numeric argument required", option);
      return;
    }

    start = max(history_base, history_base + history_length - count);
  }

  for (int i = start; i < history_base + history_length; i++) {
    if (HIST_ENTRY* historical_entry = history_get(i)) println(cout, "{:>5}  {}", i, historical_entry->line);
  }
}

using BuiltinHandler = function<void(const vector<string>&)>;

inline const unordered_map<string, BuiltinHandler> builtins{
    {"cd", handle_cd},     {"complete", handle_complete}, {"echo", handle_echo}, {"exit", handle_exit},
    {"jobs", handle_jobs}, {"pwd", handle_pwd},           {"type", handle_type}, {"history", handle_history}};

inline const BuiltinHandler* find_builtin(const string& name) {
  auto builtin = builtins.find(name);
  return builtin == builtins.end() ? nullptr : &builtin->second;
}