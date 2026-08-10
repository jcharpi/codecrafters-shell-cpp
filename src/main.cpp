#include <iostream>
#include <string>
#include <unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include <utility>  // move
#include <vector>

#include "ArgSplitter.hpp"
#include "Builtins.hpp"
#include "Command.hpp"
#include "Completion.hpp"
#include "Executables.hpp"
#include "Process.hpp"

int main() {
  cout << unitbuf;
  cerr << unitbuf;

  setup_completion();

  string input;
  while (true) {
    char* line = readline("$ ");
    if (!line) {
      break;
    }
    input = line;
    free(line);

    Command command = parse_redirection(split_args(input));
    vector<string> args = std::move(command.args);
    if (args.empty()) {
      continue;
    }

    const string& command_name = args[0];

    int saved_out = redirect_stream(STDOUT_FILENO, command.stdout_redirect);
    int saved_err = redirect_stream(STDERR_FILENO, command.stderr_redirect);

    if (auto builtin = builtins.find(command_name); builtin != builtins.end()) {
      builtin->second(args);
    } else if (string file_path = executable_in_path(command_name); !file_path.empty()) {
      handle_executable(file_path, args);
    } else {
      cout << command_name << ": command not found\n";
    }

    restore_stream(STDOUT_FILENO, saved_out);
    restore_stream(STDERR_FILENO, saved_err);
  }
}
