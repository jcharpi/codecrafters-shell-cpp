#pragma once

#include <cstdio>
#include <format>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

inline vector<string> non_empty_lines(const string& text) {
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

inline string quoted(const string& word) {
  string result = "'";
  for (char character : word)
    result += (character == '\'') ? ("'\\''") : string(1, character);
  return result + "'";
}

inline vector<string> run_completer_script(const string& script_path, const string& command, const string& curr_word,
                                           const string& prev_word, const string& COMP_LINE, size_t COMP_POINT) {
  string command_line = format("COMP_LINE={} COMP_POINT={} {} {} {} {}",
    quoted(COMP_LINE), COMP_POINT,
    quoted(script_path), quoted(command),
    quoted(curr_word), quoted(prev_word));
  FILE* script_stream = popen(command_line.c_str(), "r");
  if (!script_stream) {
    return {};
  }

  string output;
  for (int character = fgetc(script_stream); character != EOF; character = fgetc(script_stream)) {
    output += static_cast<char>(character);
  }
  pclose(script_stream);

  return non_empty_lines(output);
}
