#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "ArgSplitter.hpp"

#ifdef _WIN32
constexpr char PATH_DELIMITER = ';';
#else
constexpr char PATH_DELIMITER = ':';
#endif

bool is_file_executable(const std::string &path) {
  return access(path.c_str(), X_OK) == 0;
}

std::string executable_in_path(const std::string &file_name) {
  if (file_name.empty())
    return "";

  const char *raw_path = std::getenv("PATH");
  if (!raw_path)
    return "";

  std::stringstream path_stream(raw_path);
  std::string directory;
  while (std::getline(path_stream, directory, PATH_DELIMITER)) {
    std::string full_path =
        (std::filesystem::path(directory) / file_name).string();
    if (is_file_executable(full_path)) {
      return full_path;
    }
  }
  return "";
}

void handle_cd(const std::vector<std::string> &args) {
  if (args.size() < 2)
    return;
  const std::string &path = args[1];
  if (path == "~") {
    const char *home = std::getenv("HOME");
    if (home) {
      std::filesystem::current_path(home);
    }
  } else if (std::filesystem::is_directory(path)) {
    std::filesystem::current_path(path);
  } else {
    std::cerr << "cd: " << path << ": No such file or directory\n";
  }
}

void handle_echo(const std::vector<std::string> &args) {
  for (size_t i = 1; i < args.size(); i++) {
    std::cout << (i > 1 ? " " : "") << args[i];
  }
  std::cout << "\n";
}

void handle_exit(const std::vector<std::string> &) { std::exit(EXIT_SUCCESS); }

void handle_pwd(const std::vector<std::string> &) {
  std::cout << std::filesystem::current_path().string() << "\n";
}

void handle_type(const std::vector<std::string> &args);

const std::unordered_map<
    std::string, std::function<void(const std::vector<std::string> &args)>>
    builtins{
        {"cd", handle_cd},   {"echo", handle_echo}, {"exit", handle_exit},
        {"pwd", handle_pwd}, {"type", handle_type},
    };

void handle_type(const std::vector<std::string> &args) {
  const std::string &target = args.size() > 1 ? args[1] : "";

  if (builtins.contains(target)) {
    std::cout << target << " is a shell builtin\n";
  } else if (std::string file_path = executable_in_path(target);
             !file_path.empty()) {
    std::cout << target << " is " << file_path << "\n";
  } else {
    std::cout << target << ": not found\n";
  }
}

// args must be non-const: data() returns char* only on non-const strings,
// required by execvp
void handle_executable(const std::string &file_path,
                       std::vector<std::string> &args) {
  std::vector<char *> argv;
  argv.reserve(args.size() + 1); // +1 for the null terminator execvp requires
  for (std::string &arg : args) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);

  pid_t pid = fork();
  if (pid == -1) {
    std::cerr << "handle_executable(): fork failed\n";
    return;
  }

  if (pid == 0) { // child process (pid == 0 is the POSIX convention for the
                  // forked child)
    execvp(file_path.c_str(), argv.data());
    std::cerr << "handle_executable(): execute failed\n";
    std::exit(EXIT_FAILURE);
  } else { // parent process
    if (waitpid(pid, nullptr, 0) == -1) {
      std::cerr << "handle_executable(): wait failed\n";
    }
  }
}

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string input;
  while (true) {
    std::cout << "$ ";
    if (!std::getline(std::cin, input))
      break;

    std::vector<std::string> args = split_args(input);
    if (args.empty()) {
      continue;
    }

    const std::string &cmd = args[0];

    if (auto it = builtins.find(cmd); it != builtins.end()) {
      it->second(args);
    } else if (std::string file_path = executable_in_path(cmd);
               !file_path.empty()) {
      handle_executable(file_path, args);
    } else {
      std::cout << cmd << ": command not found\n";
    }
  }
}
