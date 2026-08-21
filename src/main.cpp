#include <cstdlib> // free
#include <iostream>
#include <print>
#include <ranges>
#include <readline/readline.h>
#include <string>
#include <unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include <utility>  // move
#include <vector>

#include "ArgSplitter.hpp"
#include "Builtins.hpp"
#include "Command.hpp"
#include "Completion.hpp"
#include "Executables.hpp"
#include "Jobs.hpp"
#include "Process.hpp"
#include "Redirection.hpp"

int main() {
  cout << unitbuf;
  cerr << unitbuf;

  setup_completion();

  string input;
  while (true) {
    for (ptrdiff_t index : background_job_display_order()) {
      if (background_jobs[index].status == JobStatus::Done) print_job_line(index);
    }
    
    char* line = readline("$ ");
    if (!line) {
      break;
    }
    input = line;
    free(line);

    Command command = parse_command(split_args(input));
    vector<string> args = std::move(command.args);
    if (args.empty()) {
      continue;
    }

    const string& command_name = args[0];

    int saved_out = redirect_stream(STDOUT_FILENO, command.stdout_redirect);
    int saved_err = redirect_stream(STDERR_FILENO, command.stderr_redirect);
    pid_t background_pid = -1;

    if (auto builtin = builtins.find(command_name); builtin != builtins.end()) {
      builtin->second(args);
    } else if (string file_path = executable_in_path(command_name); !file_path.empty()) {
      if (command.background) {
        background_pid = spawn_executable(file_path, args);
      } else {
        handle_executable(file_path, args);
      }
    } else {
      println(cout, "{}: command not found", command_name);
    }

    restore_stream(STDOUT_FILENO, saved_out);
    restore_stream(STDERR_FILENO, saved_err);

    if (background_pid != -1) {
      string command_line = args | views::join_with(' ') | ranges::to<string>();
      int job_number = add_job(background_pid, command_line);
      println(cout, "[{}] {}", job_number, background_pid);
    }
  }
}
