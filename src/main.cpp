#include <iostream>
#include <string>
#include <format>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  while (true) {
    std::cout << "$ ";
    std::string input;
    std::getline(std::cin, input);

    if (input == "exit") {
      break;
    } else if (input.substr(0, 4) == "echo") {
      std::cout << input.substr(6, input.size()) << "\n";
    } else {
      std::cout << std::format("{}: command not found\n", input);
    }
  }
}
