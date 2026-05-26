#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <format>
#include <vector>
#include <algorithm>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>

#ifdef _WIN32 
constexpr char PATH_DELIMITER = ';';
#else
constexpr char PATH_DELIMITER = ':';
#endif

const std::vector<std::string> builtins = {"exit", "echo", "type", "pwd"};

bool isFileExecutable(const std::string& path) {
  return access(path.c_str(), X_OK) == 0;
}

std::string executableInPATH(const std::string& fileName) {
  const char* directories = std::getenv("PATH");
  if (!directories) return "";

  std::stringstream ss_directories(directories);
  std::string directory;
  while (std::getline(ss_directories, directory, PATH_DELIMITER)) {
    std::string fullPath = directory + "/" + fileName;
    if (isFileExecutable(fullPath)) {
      return fullPath;
    }
  }
  return "";
}

void handleBuiltin(std::string cmd, std::string args) {
  if (cmd == "exit") {
    std::exit(EXIT_SUCCESS);
  } else if (cmd == "echo") {
    std::cout << args << "\n";
  } else if (cmd == "pwd") {
    std::cout << std::filesystem::current_path() << "\n";
  } else if (cmd == "type") {
    if (std::find(builtins.begin(), builtins.end(), args) != builtins.end()) { // Builtin
      std::cout << args << " is a shell builtin\n";
    } else { // Executable
      std::string executablePath = executableInPATH(args);
      if (!executablePath.empty()) {
        std::cout << args << " is " << executablePath << "\n";
      } else {
        std::cout << args << ": not found\n";
      }
    }
  }
}

void handleExecutable(std::string cmd, std::string args, std::string file) {
  // Split args into array
  std::vector<std::string> argsList {cmd};
  std::stringstream ss_args(args);
  for (std::string arg; ss_args >> arg;) {
    argsList.push_back(arg);
  }

  // Convert string array of args to char* array for C execvp method
  std::vector<char*> argv;
  argv.reserve(argsList.size() + 1); // +1 for the nullptr at the end
  for (std::string& arg : argsList) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);

  // Stage 1: Fork shell process into two processes
  pid_t pid = fork();
  if (pid == -1) {
    std::cerr << "handleExecutable(): fork failed\n";
    return;
  }

  if (pid == 0) { // Child process
    // Stage 2: Execute
    execvp(file.c_str(), argv.data());
    std::cerr << "handleExecutable(): execute failed\n";
    std::exit(EXIT_FAILURE);
  } else { // Main, parent process
    // Stage 3: Wait for child to execute
    if (waitpid(pid, nullptr, 0) == -1) {
      std::cerr << "handleExecutable(): wait failed\n";
    }
  }
}

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string input;
  while (true) {
    std::cout << "$ ";
    if (!std::getline(std::cin, input)) break;

    std::string cmd = input.substr(0, input.find(" "));
    std::string args = input.size() > cmd.size() ? input.substr(cmd.size() + 1) : "";
    if (std::find(builtins.begin(), builtins.end(), cmd) != builtins.end()) {
      handleBuiltin(cmd, args);
    } else if (std::string file = executableInPATH(cmd); !file.empty()) {
      handleExecutable(cmd, args, file);
    } else { // Unrecognized
      std::cout << std::format("{}: command not found\n", cmd);
    }
  }
}
