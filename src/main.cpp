#include <cstdlib>
#include <iostream>
#include <string>
#include <format>
#include <vector>
#include <algorithm>
#include <sstream>
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

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  const std::vector<std::string> builtins = {"exit", "echo", "type"};

  std::string input;
  while (true) {
    std::cout << "$ ";
    if (!std::getline(std::cin, input)) break;

    std::string cmd = input.substr(0, input.find(" "));
    std::string args = input.size() > cmd.size() ? input.substr(cmd.size() + 1) : "";

    if (cmd == "exit") {
      break;
    } else if (cmd == "echo") {
      std::cout << args << "\n";
    } else if (cmd == "type") {
      // Builtin
      if (std::find(builtins.begin(), builtins.end(), args) != builtins.end()) {
        std::cout << args << " is a shell builtin\n";
      } else { // Executable
        std::string executablePath = executableInPATH(args);
        if (!executablePath.empty()) {
          std::cout << args << " is " << executablePath << "\n";
        } else {
          std::cout << args << ": not found\n";
        }
      }
    } else { // Unrecognized
      std::cout << std::format("{}: command not found\n", cmd);
    }
  }
}
