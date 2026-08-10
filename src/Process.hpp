#pragma once

#include <cstdlib> // exit, EXIT_FAILURE
#include <iostream>
#include <string>
#include <sys/wait.h> // waitpid
#include <unistd.h>   // execvp, fork
#include <vector>

using namespace std;

// Starting child processes and waiting on them. Knows nothing about PATH
// lookup (Executables.hpp) or about what the child is for.

// args must be non-const: data() returns char* only on non-const strings,
// required by execvp
inline void handle_executable(const string &file_path, vector<string> &args) {
  vector<char *> argv;
  argv.reserve(ssize(args) + 1); // +1 for the null terminator execvp requires
  for (string &arg : args) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);

  pid_t pid = fork();
  if (pid == -1) {
    cerr << "handle_executable(): fork failed\n";
    return;
  }

  // pid == 0 is the POSIX convention for the forked child
  if (pid == 0) {
    execvp(file_path.c_str(), argv.data());
    cerr << "handle_executable(): execute failed\n";
    exit(EXIT_FAILURE);
  } else {
    if (waitpid(pid, nullptr, 0) == -1) {
      cerr << "handle_executable(): wait failed\n";
    }
  }
}
