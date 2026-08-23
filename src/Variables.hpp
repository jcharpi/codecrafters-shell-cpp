#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <ranges>
#include <string>

using namespace std;

inline map<string, string> shell_variables;

inline bool is_valid_start(char character) {
  return isalpha(static_cast<unsigned char>(character)) || character == '_';
}

inline bool is_valid_name_char(char character) {
  return isalnum(static_cast<unsigned char>(character)) || character == '_';
}

inline bool is_valid_identifier(const string& name) {
  return !name.empty() && is_valid_start(name[0]) && ranges::all_of(name | views::drop(1), is_valid_name_char);
}
