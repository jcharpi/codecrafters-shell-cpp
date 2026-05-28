#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <format>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>

#ifdef _WIN32 
constexpr char PATH_DELIMITER = ';';
#else
constexpr char PATH_DELIMITER = ':';
#endif

bool isFileExecutable(const std::string& path) {
  return access(path.c_str(), X_OK) == 0;
}

std::string executableInPATH(const std::string& fileName) {
  const char* directories = std::getenv("PATH");
  if (!directories) return "";

  std::stringstream ss_directories(directories);
  std::string directory;
  while (std::getline(ss_directories, directory, PATH_DELIMITER)) {
    std::string fullPath = (std::filesystem::path(directory) / fileName).string();
    if (isFileExecutable(fullPath)) {
      return fullPath;
    }
  }
  return "";
}

bool isBuiltin(const std::string& cmd);

void handleExit(const std::vector<std::string>& args) {
  std::exit(EXIT_SUCCESS);
}

void handleEcho(const std::vector<std::string>& args) {
  for (size_t i = 1; i < args.size(); i++) {
    std::cout << (i > 1 ? " " : "") << args[i];
  }
  std::cout << "\n";
}

void handlePwd(const std::vector<std::string>& args) {
  std::cout << std::filesystem::current_path().string() << "\n";
}

void handleType(const std::vector<std::string>& args) {
  const std::string& target = args.size() > 1 ? args[1] : "";

  if (isBuiltin(target)) {
    std::cout << target << " is a shell builtin\n";
  } else if (std::string file_path = executableInPATH(target); !file_path.empty()) {
    std::cout << target << " is " << file_path << "\n";
  } else {
    std::cout << target << ": not found\n";
  }
}

const std::unordered_map<std::string, std::function<void(const std::vector<std::string>& args)>> builtins {
  {"exit", handleExit},
  {"echo", handleEcho},
  {"pwd", handlePwd},
  {"type", handleType},
};

bool isBuiltin(const std::string& cmd) {
  return builtins.contains(cmd);
}

void handleBuiltin(const std::vector<std::string>& args) {
  if (auto it = builtins.find(args[0]); it != builtins.end()) {
    it->second(args);
  }
}

std::vector<std::string> splitArgs(const std::string& input) {
  std::vector<std::string> argsList;
  std::stringstream ss_args(input);
  for (std::string arg; ss_args >> arg;) {
    argsList.push_back(arg);
  }
  return argsList;
}

void handleExecutable(const std::string& file_path, std::vector<std::string>& args) {
  std::vector<char*> argv;
  argv.reserve(args.size() + 1); // +1 for the nullptr at the end
  for (std::string& arg : args) {
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
    execvp(file_path.c_str(), argv.data());
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

    std::vector<std::string> args = splitArgs(input);
    if (args.empty()) {
      continue;
    }

    const std::string& cmd = args[0];

    if (isBuiltin(cmd)) {
      handleBuiltin(args);
    } else if (std::string file_path = executableInPATH(cmd); !file_path.empty()) {
      handleExecutable(file_path, args);
    } else {
      std::cout << std::format("{}: command not found\n", cmd);
    }
  }
}
