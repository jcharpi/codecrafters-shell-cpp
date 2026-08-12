#pragma once

#include <cstdlib> // exit, EXIT_FAILURE
#include <iostream>
#include <print>
#include <string>
#include <sys/wait.h> // waitpid
#include <unistd.h>   // execvp, fork
#include <vector>

using namespace std;

inline pid_t spawn_executable(const string& file_path, vector<string>& args) {
  vector<char*> argv;
  argv.reserve(ssize(args) + 1); // +1 for the null terminator execvp requires
  for (string& arg : args) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);

  pid_t pid = fork();
  if (pid == -1) {
    println(cerr, "spawn_executable(): fork failed");
    return pid;
  }

  if (pid == 0) {
    execvp(file_path.c_str(), argv.data());
    println(cerr, "spawn_executable(): execute failed");
    exit(EXIT_FAILURE);
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
