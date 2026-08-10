#pragma once

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

inline vector<string> non_empty_lines(const string &text) {
  vector<string> lines;
  istringstream text_stream(text);
  string line;
  while (getline(text_stream, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  return lines;
}

// Runs a `complete -C` script and returns the candidates it prints.
inline vector<string> run_completer_script(const string &script_path) {
  FILE *script_stream = popen(script_path.c_str(), "r");
  if (!script_stream) {
    return {};
  }

  string output;
  for (int character = fgetc(script_stream); character != EOF;
       character = fgetc(script_stream)) {
    output += static_cast<char>(character);
  }
  pclose(script_stream);

  return non_empty_lines(output);
}
