#pragma once

#include <cstdlib>
#include <unistd.h>
#include <vector>

#include "Builtins.hpp"
#include "Command.hpp"
#include "Executables.hpp"
#include "Process.hpp"
#include "Redirection.hpp"

using namespace std;

// A pipe is a chunk of memory the OS holds for us.
// One process reads out of that chunk [0], and another writes to it [1].
struct Pipe {
  int read_fd = -1; // -1 means "no pipe", so every check below is the same shape
  int write_fd = -1;
};

inline Pipe make_pipe() {
  int fds[2];
  if (pipe(fds) == -1) {
    println(cerr, "make_pipe(): pipe failed");
    return {};
  }
  return Pipe{fds[0], fds[1]};
}

// Runs one command of a pipeline in a forked child. stdin_fd is the read end left by
// the previous command (-1 for the first), to_next is the pipe this command writes
// into (both ends -1 for the last). Returns the child's pid to the parent.
inline pid_t spawn_pipeline_stage(Command& command, int stdin_fd, const Pipe& to_next) {
  pid_t pid = fork();
  if (pid == -1) {
    println(cerr, "spawn_pipeline_stage(): fork failed");
    return pid;
  }

  if (pid != 0) return pid;

  // The fork copied both ends of that pipe to us, but this command only writes into it;
  // the read end belongs to the next command.
  if (to_next.read_fd != -1) close(to_next.read_fd);

  // Point our stdin slot at the previous command's pipe and our stdout slot at the
  // next one.
  attach_fd(stdin_fd, STDIN_FILENO);
  attach_fd(to_next.write_fd, STDOUT_FILENO);

  // A redirect on this command overwrites the slot, beating out the pipe.
  // Ex. "ls > out | wc" that leaves ls writing to out and nothing entering the pipe,
  // so wc reads EOF and reports 0.
  redirect_stream(STDOUT_FILENO, command.stdout_redirect);
  redirect_stream(STDERR_FILENO, command.stderr_redirect);

   if (command.args.empty()) _exit(EXIT_SUCCESS);
  const string& command_name = command.args[0];

  if (const BuiltinHandler* builtin_handler = find_builtin(command_name)) {
    (*builtin_handler)(command.args);
    cout.flush();
    _exit(EXIT_SUCCESS);
  }

  string file_path = executable_in_path(command_name);
  if (file_path.empty()) {
    println(cerr, "{}: command not found", command_name);
    _exit(EXIT_FAILURE);
  }

  exec_command(file_path, command.args);
  println(cerr, "spawn_pipeline_stage(): execute failed");
  _exit(EXIT_FAILURE);
}

// Called once with every command, it forks one child per
// command and chains them together. For "ls -l | grep foo | wc":
//
//   run_pipeline([ls -l][grep foo][wc])
//     spawn_pipeline_stage(ls -l,    stdin = -1,       to_next = P1)
//     spawn_pipeline_stage(grep foo, stdin = P1.read,  to_next = P2)
//     spawn_pipeline_stage(wc,       stdin = P2.read,  to_next = {})
//     waitpid all three
inline void run_pipeline(vector<Command>& commands) {
  vector<pid_t> pids;
  pids.reserve(commands.size());
  int pipeline_stdin_fd = -1;

  for (Command& command : commands) {
    bool has_next = &command != &commands.back();
    Pipe to_next = has_next ? make_pipe() : Pipe{};

    pids.push_back(spawn_pipeline_stage(command, pipeline_stdin_fd, to_next));

    // The child has its own copies now; close copies in shell to prevent
    // commands from hanging rather than reporting EOF
    if (pipeline_stdin_fd != -1) close(pipeline_stdin_fd);
    if (to_next.write_fd != -1) close(to_next.write_fd);

    pipeline_stdin_fd = to_next.read_fd;
  }

  for (pid_t pid : pids) {
    if (pid != -1) waitpid(pid, nullptr, 0);
  }
}
