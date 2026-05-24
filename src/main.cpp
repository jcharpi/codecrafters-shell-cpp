#include <iostream>
#include <string>
#include <format>
#include <vector>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  while (true) {
    std::cout << "$ ";
    std::string input;
    std::getline(std::cin, input);

    std::vector<std::string> builtins = {"exit", "echo", "type"};

    if (input == "exit") {
      break;
    } else if (input.substr(0, 4) == "echo") {
      std::cout << input.substr(5, input.size()) << "\n";
    } else if (input.substr(0, 4) == "type") {
      std::string command = input.substr(5, input.size());
      if (std::find(builtins.begin(), builtins.end(), command) != builtins.end()) {
        std::cout << command << " is a shell builtin\n";
      } else {
        std::cout << command << ": not found\n";
      }
    } else {
      std::cout << std::format("{}: command not found\n", input);
    }
  }
}
