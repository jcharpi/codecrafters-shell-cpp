#pragma once

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
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

inline optional<int> parse_int(const string& text) {
  int value = 0;
  const char* last = text.c_str() + ssize(text);
  auto [end, error] = from_chars(text.c_str(), last, value);
  if (error != errc{} || end != last) return nullopt;
  return value;
}

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

inline bool in_subshell = false;
inline void handle_exit(const vector<string>& args) {
  int status = EXIT_SUCCESS;
  if (ssize(args) > 1) {
    if (optional<int> parsed = parse_int(args[1])) {
      status = *parsed;
    } else {
      println(cerr, "exit: {}: numeric argument required", args[1]);
      status = 2;
    }
  }
  if (in_subshell) _exit(status);
  exit(status);
}

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

inline int written_history_marker = 0;

inline void load_history_file(const string& file_path) {
  read_history(file_path.c_str());
  written_history_marker = history_length;
}

inline void save_history_file(const string& file_path) {
  write_history(file_path.c_str());
  written_history_marker = history_length;
}

inline void append_history_file(const string& file_path) {
  int pending_history_marker = history_length - written_history_marker;
  if (pending_history_marker <= 0) return;

  // if file to append to doesn't exist, create it, then retry
  if (append_history(pending_history_marker, file_path.c_str()) != 0) {
    ofstream create_file{file_path, ios::app};
    create_file.close();
    append_history(pending_history_marker, file_path.c_str());
  }

  written_history_marker = history_length;
}

inline void handle_history(const vector<string>& args) {
  const string& option = ssize(args) > 1 ? args[1] : "";
  if (option == "-r" || option == "-w" || option == "-a") {
    const char* history_file = getenv("HISTFILE");
    // use provided path, otherwise use HISTFILE as fallback
    string file_path = ssize(args) >= 3 ? args[2] : (history_file ? history_file : "");
    if (file_path.empty()) return;

    if (option == "-r")
      load_history_file(file_path);
    else if (option == "-w")
      save_history_file(file_path);
    else
      append_history_file(file_path);
    return;
  }

  int start = history_base;
  if (!option.empty()) {
    optional<int> count = parse_int(option);
    if (!count || *count < 0) {
      println(cerr, "history: {}: numeric argument required", option);
      return;
    }

    start = max(history_base, history_base + history_length - *count);
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