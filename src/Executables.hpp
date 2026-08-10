#pragma once

#include <cstdlib> // getenv
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h> // access, X_OK
#include <unordered_set>
#include <vector>

using namespace std;

// Locating external programs and directory entries on disk. Running them is
// Process.hpp's job; builtins are Builtins.hpp's.

#ifdef _WIN32
constexpr char PATH_DELIMITER = ';';
#else
constexpr char PATH_DELIMITER = ':';
#endif

inline bool is_file_executable(const string& path) { return access(path.c_str(), X_OK) == 0; }

inline vector<string> directories_in_path() {
  const char* raw_path = getenv("PATH");
  if (!raw_path) return {};

  vector<string> directories;
  stringstream path_stream(raw_path);
  string directory;
  while (getline(path_stream, directory, PATH_DELIMITER)) {
    directories.push_back(directory);
  }

  return directories;
}

// Returns the full path to `file_name` if it's an executable somewhere on PATH,
// or "" if not found.
inline string executable_in_path(const string& file_name) {
  if (file_name.empty()) return "";

  for (const string& directory : directories_in_path()) {
    string full_path = (filesystem::path(directory) / file_name).string();
    if (is_file_executable(full_path)) {
      return full_path;
    }
  }

  return "";
}

inline vector<string> executable_names_in_path() {
  unordered_set<string> executable_names;

  for (const string& directory : directories_in_path()) {
    error_code error;
    for (const auto& directory_entry : filesystem::directory_iterator(directory, error)) {
      if (is_file_executable(directory_entry.path().string())) {
        executable_names.insert(directory_entry.path().filename().string());
      }
    }
  }

  return vector<string>(executable_names.begin(), executable_names.end());
}

inline vector<string> entries_in_directory(const string& partial_path) {
  filesystem::path path(partial_path);
  filesystem::path directory = path.parent_path();
  unordered_set<string> entries;

  error_code error;
  for (const auto& directory_entry : filesystem::directory_iterator(directory.empty() ? "." : directory, error)) {
    if (!directory_entry.is_regular_file() && !directory_entry.is_directory()) {
      continue;
    }

    string entry = (directory / directory_entry.path().filename()).string();
    if (directory_entry.is_directory()) entry += '/'; // shell uses this regardless of platform
    entries.insert(entry);
  }

  return vector<string>(entries.begin(), entries.end());
}
