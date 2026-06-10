#pragma once

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <optional>
#include <readline/history.h>
#include <readline/readline.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

constexpr int NEW_FILE_PERMISSIONS = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

struct Redirect {
  std::string file;
  bool append = false;
};

struct Command {
  std::vector<std::string> args;
  std::optional<Redirect> stdout_redirect;
  std::optional<Redirect> stderr_redirect;
};

inline Command parse_redirection(std::vector<std::string> args) {
  Command cmd;
  for (size_t i = 0; i < args.size(); i++) {
    const std::string &arg = args[i];

    std::optional<Redirect> *target = nullptr;
    bool append = false;
    if (arg == ">" || arg == "1>") {
      target = &cmd.stdout_redirect;
    } else if (arg == ">>" || arg == "1>>") {
      target = &cmd.stdout_redirect;
      append = true;
    } else if (arg == "2>") {
      target = &cmd.stderr_redirect;
    } else if (arg == "2>>") {
      target = &cmd.stderr_redirect;
      append = true;
    }

    if (target && i + 1 < args.size()) {
      *target = Redirect{std::move(args[++i]), append};
    } else {
      cmd.args.push_back(std::move(args[i]));
    }
  }
  return cmd;
}

inline int open_redirect_file(const Redirect &redirect) {
  int open_permissions =
      O_WRONLY | O_CREAT | (redirect.append ? O_APPEND : O_TRUNC);
  return open(redirect.file.c_str(), open_permissions, NEW_FILE_PERMISSIONS);
}

inline int redirect_stream(int curr_stream_fd,
                           const std::optional<Redirect> &redirect) {
  if (!redirect) {
    return -1;
  }

  int redirect_fd = open_redirect_file(*redirect);
  if (redirect_fd == -1) {
    std::cerr << "redirect_stream(): " << redirect->file << "\n";
    return -1;
  }

  int original_stream_fd = dup(curr_stream_fd);
  dup2(redirect_fd, curr_stream_fd);
  close(redirect_fd);
  return original_stream_fd;
}

inline void restore_stream(int curr_stream_fd, int original_stream_fd) {
  if (original_stream_fd == -1) {
    return;
  }
  dup2(original_stream_fd, curr_stream_fd);
  close(original_stream_fd);
}