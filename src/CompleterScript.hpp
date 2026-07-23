#pragma once

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

inline std::vector<std::string> non_empty_lines(const std::string &text) {
  std::vector<std::string> lines;
  std::istringstream text_stream(text);
  std::string line;
  while (std::getline(text_stream, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  return lines;
}

inline std::vector<std::string>
run_completer_script(const std::string &script_path) {
  // popen starts the script now and gives us a stream to its stdout.
  FILE *script_stream = popen(script_path.c_str(), "r");
  if (!script_stream) {
    return {};
  }

  std::string output;
  for (int character = fgetc(script_stream); character != EOF;
       character = fgetc(script_stream)) {
    output += static_cast<char>(character);
  }
  pclose(script_stream);

  return non_empty_lines(output);
}
