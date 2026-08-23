#include <cstdlib>
#include <iostream>
#include <print>
#include <ranges>
#include <readline/history.h>
#include <readline/readline.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "ArgSplitter.hpp"
#include "Builtins.hpp"
#include "Command.hpp"
#include "Completion.hpp"
#include "Executables.hpp"
#include "Jobs.hpp"
#include "Pipeline.hpp"
#include "Process.hpp"
#include "Redirection.hpp"

int main() {
  cout << unitbuf;
  cerr << unitbuf;

  setup_completion();
  if (const char* history_file = getenv("HISTFILE")) load_history_file(history_file);

  string input;
  while (true) {
    reap_background_jobs();
    for (ptrdiff_t index : background_job_display_order()) {
      if (background_jobs[index].status == JobStatus::Done) print_job_line(index);
    }
    purge_done_jobs();

    char* line = readline("$ ");
    if (!line) {
      break;
    }
    input = line;
    free(line);

    if (input.find_first_not_of(" \t") != string::npos) add_history(input.c_str());

    Pipeline pipeline = parse_pipeline(split_args(input));
    if (ssize(pipeline.commands) > 1) {
      run_pipeline(pipeline.commands);
      continue;
    }

    Command command = std::move(pipeline.commands.front());
    vector<string> args = std::move(command.args);
    if (args.empty()) {
      continue;
    }

    const string& command_name = args[0];

    int saved_out = redirect_stream(STDOUT_FILENO, command.stdout_redirect);
    int saved_err = redirect_stream(STDERR_FILENO, command.stderr_redirect);
    pid_t background_pid = -1;

    if (const BuiltinHandler* builtin_handler = find_builtin(command_name)) {
      (*builtin_handler)(args);
    } else if (string file_path = executable_in_path(command_name); !file_path.empty()) {
      if (pipeline.background) {
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
