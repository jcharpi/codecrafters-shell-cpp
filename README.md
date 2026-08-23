# cpp-shell

A POSIX shell built from scratch in C++23 for the CodeCrafters
["Build Your Own Shell"](https://codecrafters.io/challenges/shell) challenge.
Every stage and every extension passed: quoting, redirection, autocompletion,
pipelines, history, job control, and shell variables.

You can try it without building anything. It runs in a sandbox on my site at
[joshcharpentier.dev](https://joshcharpentier.dev).

## What it does

- REPL on GNU Readline: prompt, line editing, arrow-key history
- Builtins: `cd` `echo` `exit` `pwd` `type` `history` `jobs` `declare` `complete`
- External commands: PATH search, then fork, execvp, waitpid
- Quoting: single quotes are literal, double quotes allow escapes and expansion,
  and adjacent quoted/unquoted pieces join into one argument
- Variables: `declare name=value`, `$name` and `${name}` expansion, falling back
  to the environment
- Redirection: `>` `>>` `1>` `1>>` `2>` `2>>`, built on open, dup, and dup2, and
  restored after each command
- Pipelines: one forked child per stage, kernel pipes between them, every spare
  file descriptor closed so EOF actually arrives
- Background jobs: trailing `&`, a jobs table with `+`/`-` markers, non-blocking
  reaping with `waitpid(WNOHANG)` at every prompt
- History: `history [n]`, HISTFILE load and save, `history -r/-w/-a`
- Tab completion: builtins and PATH executables at the command position,
  filesystem entries for arguments (directories get a `/` and no trailing space,
  so Tab keeps descending), plus completer scripts registered with `complete -C`
  and called bash-style with COMP_LINE and COMP_POINT

## How it's laid out

One header per concern. `main.cpp` is just the loop.

| File | What it owns |
| --- | --- |
| `main.cpp` | The REPL: reap jobs, read a line, parse, dispatch, restore streams |
| `ArgSplitter.hpp` | The lexer, a cursor over the raw line with one method per quoting rule |
| `Command.hpp` | Turning tokens into a Command or Pipeline: args, redirects, `&` |
| `Redirection.hpp` | Pointing a standard stream at a file and putting it back after |
| `Pipeline.hpp` | Forking each stage, wiring the pipes, closing the copies that cause hangs |
| `Process.hpp` | fork, execvp, and waitpid for one external command |
| `Builtins.hpp` | The nine builtins and their argument handling |
| `Executables.hpp` | PATH search and directory listing |
| `Jobs.hpp` | The background job table: numbering, markers, reaping, purging |
| `Variables.hpp` | The shell variable store and identifier rules |
| `Completion.hpp` | Readline wiring: generators and the completion dispatcher |
| `CompleterScript.hpp` | Running `complete -C` scripts and collecting their output |
| `CompletionRegistry.hpp` | The command-to-completer-script map |

## Things I had to learn the hard way

- What actually happens between pressing Enter and seeing output: the shell
  forks a copy of itself, execvp swaps in the program, waitpid collects it
- dup2 is the whole trick behind redirection. Point fd 1 at a file or a pipe
  and the program on the other end never knows the difference
- A pipeline hangs when anyone forgets to close a write end, because a reader
  only sees EOF once every write end in every process is gone
- Finished children sit in the process table as zombies until someone calls
  waitpid, so the shell reaps with WNOHANG at the top of every loop
- A forked child that calls `exit()` instead of `_exit()` flushes the parent's
  inherited stdio buffers and prints things twice
- Readline completion is a callback dance: it calls your generator once per
  candidate and frees whatever you return, so the strings had better be strdup'd

## Build and run

Needs CMake and GNU Readline (`brew install cmake readline` on macOS).

```sh
cmake -B build
cmake --build build
./build/shell
```

Or through the CodeCrafters wrapper: `./your_program.sh`
