#pragma once

#include <fcntl.h>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

// Pointing a stream at a file and putting it back afterwards. Recognising the
// tokens that describe a redirect is Command.hpp's job.

constexpr int NEW_FILE_PERMISSIONS = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

struct Redirect {
  string file;
  bool append = false;
};

inline int open_redirect_file(const Redirect& redirect) {
  int open_permissions = O_WRONLY | O_CREAT | (redirect.append ? O_APPEND : O_TRUNC);
  return open(redirect.file.c_str(), open_permissions, NEW_FILE_PERMISSIONS);
}

// Makes a standard stream read from or write to somewhere else.
// Useful for handing a child a pipe or a file, since it execs a program that knows nothing
// about our fds and will only ever use stdin and stdout.
inline void attach_fd(int source_fd, int stream_fd) {
  if (source_fd == -1) return;
  dup2(source_fd, stream_fd);
  close(source_fd); // dup2 gave us a copy at stream_fd, so the original is dead weight
}

inline int redirect_stream(int curr_stream_fd, const optional<Redirect>& redirect) {
  if (!redirect) {
    return -1;
  }

  int redirect_fd = open_redirect_file(*redirect);
  if (redirect_fd == -1) {
    println(cerr, "redirect_stream(): could not open {}", redirect->file);
    return -1;
  }

  int original_stream_fd = dup(curr_stream_fd);
  attach_fd(redirect_fd, curr_stream_fd);
  return original_stream_fd;
}

inline void restore_stream(int curr_stream_fd, int original_stream_fd) {
  attach_fd(original_stream_fd, curr_stream_fd);
}
