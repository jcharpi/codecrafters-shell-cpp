#pragma once

#include <cstdlib> // std::getenv
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h> // waitpid
#include <unistd.h>   // access, fork, execvp, X_OK
#include <vector>

// Everything about *external* programs: locating them on PATH and running them.
// Knows nothing about builtins.

#ifdef _WIN32
constexpr char PATH_DELIMITER = ';';
#else
constexpr char PATH_DELIMITER = ':';
#endif

inline bool is_file_executable(const std::string &path) {
  return access(path.c_str(), X_OK) == 0;
}

// Returns the full path to `file_name` if it's an executable somewhere on PATH,
// or "" if not found.
inline std::string executable_in_path(const std::string &file_name) {
  if (file_name.empty())
    return "";

  const char *raw_path = std::getenv("PATH");
  if (!raw_path)
    return "";

  std::stringstream path_stream(raw_path);
  std::string directory;
  while (std::getline(path_stream, directory, PATH_DELIMITER)) {
    std::string full_path =
        (std::filesystem::path(directory) / file_name).string();
    if (is_file_executable(full_path)) {
      return full_path;
    }
  }
  return "";
}

// args must be non-const: data() returns char* only on non-const strings,
// required by execvp
inline void handle_executable(const std::string &file_path,
                              std::vector<std::string> &args) {
  std::vector<char *> argv;
  argv.reserve(args.size() + 1); // +1 for the null terminator execvp requires
  for (std::string &arg : args) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);

  pid_t pid = fork();
  if (pid == -1) {
    std::cerr << "handle_executable(): fork failed\n";
    return;
  }

  // child process (pid == 0 is the POSIX convention for the
  // forked child)
  if (pid == 0) {
    execvp(file_path.c_str(), argv.data());
    std::cerr << "handle_executable(): execute failed\n";
    std::exit(EXIT_FAILURE);
  } else { // parent process
    if (waitpid(pid, nullptr, 0) == -1) {
      std::cerr << "handle_executable(): wait failed\n";
    }
  }
}
