#include <iostream>
#include <string>
#include <unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include <utility>  // std::move
#include <vector>

#include "ArgSplitter.hpp"
#include "Builtins.hpp"
#include "Command.hpp"
#include "Completion.hpp"
#include "Executables.hpp"

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  setup_completion();

  std::string input;
  while (true) {
    char *line = readline("$ ");
    if (!line) {
      break;
    }
    input = line;
    free(line);

    Command command = parse_redirection(split_args(input));
    std::vector<std::string> args = std::move(command.args);
    if (args.empty()) {
      continue;
    }

    const std::string &cmd = args[0];

    int saved_out = redirect_stream(STDOUT_FILENO, command.stdout_redirect);
    int saved_err = redirect_stream(STDERR_FILENO, command.stderr_redirect);

    if (auto it = builtins.find(cmd); it != builtins.end()) {
      it->second(args);
    } else if (std::string file_path = executable_in_path(cmd);
               !file_path.empty()) {
      handle_executable(file_path, args);
    } else {
      std::cout << cmd << ": command not found\n";
    }

    restore_stream(STDOUT_FILENO, saved_out);
    restore_stream(STDERR_FILENO, saved_err);
  }
}
