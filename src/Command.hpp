#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Redirection.hpp"

using namespace std;

// Turning a token list into a Command: which words are arguments, which
// describe a redirect, and whether the line ends in `&`. Applying the redirects
// is Redirection.hpp's job; running the result is Process.hpp's.

struct Command {
  vector<string> args;
  optional<Redirect> stdout_redirect;
  optional<Redirect> stderr_redirect;
  bool background = false;
};

inline Command parse_command(vector<string> args) {
  Command command;

  if (!args.empty() && args.back() == "&") {
    command.background = true;
    args.pop_back();
  }

  for (ptrdiff_t i = 0; i < ssize(args); i++) {
    const string& arg = args[i];

    optional<Redirect>* target = nullptr;
    bool append = false;
    if (arg == ">" || arg == "1>") {
      target = &command.stdout_redirect;
    } else if (arg == ">>" || arg == "1>>") {
      target = &command.stdout_redirect;
      append = true;
    } else if (arg == "2>") {
      target = &command.stderr_redirect;
    } else if (arg == "2>>") {
      target = &command.stderr_redirect;
      append = true;
    }

    if (target && i + 1 < ssize(args)) {
      *target = Redirect{std::move(args[++i]), append};
    } else {
      command.args.push_back(std::move(args[i]));
    }
  }
  return command;
}
