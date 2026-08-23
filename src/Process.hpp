#pragma once

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <print>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

inline void exec_command(const string& file_path, vector<string>& args) {
  vector<char*> argv;
  argv.reserve(ssize(args) + 1); // +1 for the null terminator execvp requires
  for (string& arg : args) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);
  execvp(file_path.c_str(), argv.data());
}

inline pid_t spawn_executable(const string& file_path, vector<string>& args) {
  pid_t pid = fork();
  if (pid == -1) {
    println(cerr, "spawn_executable(): fork failed");
    return pid;
  }

  if (pid == 0) {
    exec_command(file_path, args);
    println(cerr, "spawn_executable(): execute failed");
    _exit(EXIT_FAILURE);
  }

  return pid;
}

inline void handle_executable(const string& file_path, vector<string>& args) {
  pid_t pid = spawn_executable(file_path, args);
  if (pid == -1) return;
  if (waitpid(pid, nullptr, 0) == -1) {
    println(cerr, "handle_executable(): wait failed");
  }
}
