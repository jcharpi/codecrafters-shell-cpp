#pragma once

#include <cstdlib> // std::getenv
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h> // waitpid
#include <unistd.h>   // access, fork, execvp, X_OK
#include <unordered_set>
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

inline std::vector<std::string> directories_in_path() {
  const char *raw_path = std::getenv("PATH");
  if (!raw_path)
    return {};

  std::vector<std::string> directories;
  std::stringstream path_stream(raw_path);
  std::string directory;
  while (std::getline(path_stream, directory, PATH_DELIMITER)) {
    directories.push_back(directory);
  }

  return directories;
}

// Returns the full path to `file_name` if it's an executable somewhere on PATH,
// or "" if not found.
inline std::string executable_in_path(const std::string &file_name) {
  if (file_name.empty())
    return "";

  for (const std::string &directory : directories_in_path()) {
    std::string full_path =
        (std::filesystem::path(directory) / file_name).string();
    if (is_file_executable(full_path)) {
      return full_path;
    }
  }

  return "";
}

inline std::vector<std::string> executable_names_in_path() {
  std::unordered_set<std::string> executable_names;

  for (const std::string &directory : directories_in_path()) {
    std::error_code error_code;
    for (const auto &directory_entry :
         std::filesystem::directory_iterator(directory, error_code)) {
      if (is_file_executable(directory_entry.path().string())) {
        executable_names.insert(directory_entry.path().filename().string());
      }
    }
  }

  return std::vector<std::string>(executable_names.begin(),
                                  executable_names.end());
}

inline std::vector<std::string> file_names_in_directory(const std::string &partial_path) {
  std::filesystem::path path(partial_path);
  std::filesystem::path directory = path.parent_path();
  std::unordered_set<std::string> file_names;

  std::error_code error_code;
  for (const auto &directory_entry :
       std::filesystem::directory_iterator(directory.empty() ? "." : directory, error_code)) {
    if (directory_entry.is_regular_file()) {
      file_names.insert((directory / directory_entry.path().filename()).string());
    }
  }

  return std::vector<std::string>(file_names.begin(), file_names.end());
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
