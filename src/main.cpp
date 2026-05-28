#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

#ifdef _WIN32
constexpr char PATH_DELIMITER = ';';
#else
constexpr char PATH_DELIMITER = ':';
#endif

bool isFileExecutable(const std::string& path) {
  return access(path.c_str(), X_OK) == 0;
}

std::string executableInPATH(const std::string& fileName) {
  const char* rawPath = std::getenv("PATH");
  if (!rawPath) return "";

  std::stringstream pathStream(rawPath);
  std::string directory;
  while (std::getline(pathStream, directory, PATH_DELIMITER)) {
    std::string fullPath = (std::filesystem::path(directory) / fileName).string();
    if (isFileExecutable(fullPath)) {
      return fullPath;
    }
  }
  return "";
}

bool isBuiltin(const std::string& cmd);

void handleExit(const std::vector<std::string>&) {
  std::exit(EXIT_SUCCESS);
}

void handleEcho(const std::vector<std::string>& args) {
  for (size_t i = 1; i < args.size(); i++) {
    std::cout << (i > 1 ? " " : "") << args[i];
  }
  std::cout << "\n";
}

void handlePwd(const std::vector<std::string>&) {
  std::cout << std::filesystem::current_path().string() << "\n";
}

void handleType(const std::vector<std::string>& args) {
  const std::string& target = args.size() > 1 ? args[1] : "";

  if (isBuiltin(target)) {
    std::cout << target << " is a shell builtin\n";
  } else if (std::string filePath = executableInPATH(target); !filePath.empty()) {
    std::cout << target << " is " << filePath << "\n";
  } else {
    std::cout << target << ": not found\n";
  }
}

const std::unordered_map<std::string, std::function<void(const std::vector<std::string>& args)>> builtins {
  {"echo", handleEcho},
  {"exit", handleExit},
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
  std::vector<std::string> args;
  std::stringstream inputStream(input);
  for (std::string arg; inputStream >> arg;) {
    args.push_back(arg);
  }
  return args;
}

// args must be non-const: data() returns char* only on non-const strings, required by execvp
void handleExecutable(const std::string& filePath, std::vector<std::string>& args) {
  std::vector<char*> argv;
  argv.reserve(args.size() + 1); // +1 for the null terminator execvp requires
  for (std::string& arg : args) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);

  pid_t pid = fork();
  if (pid == -1) {
    std::cerr << "handleExecutable(): fork failed\n";
    return;
  }

  if (pid == 0) { // child process (pid == 0 is the POSIX convention for the forked child)
    execvp(filePath.c_str(), argv.data());
    std::cerr << "handleExecutable(): execute failed\n";
    std::exit(EXIT_FAILURE);
  } else { // parent process
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
    } else if (std::string filePath = executableInPATH(cmd); !filePath.empty()) {
      handleExecutable(filePath, args);
    } else {
      std::cout << cmd << ": command not found\n";
    }
  }
}
