#pragma once

#include "Variables.hpp"
#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std;

class ArgSplitter {
public:
  explicit ArgSplitter(string_view source) : source_(source) {}

  vector<string> split() {
    vector<string> args;
    while (!at_end()) {
      skip_whitespace();
      if (at_end()) break;
      string arg = read_arg();
      if (!arg.empty()) {
        args.push_back(std::move(arg));
      }
    }
    return args;
  }

private:
  bool at_end() const { return position_ >= ssize(source_); }

  char peek() const { return at_end() ? '\0' : source_[position_]; }

  bool is_space() const { return isspace(static_cast<unsigned char>(peek())); }

  char advance() { return source_[position_++]; }

  void skip_whitespace() {
    while (!at_end() && is_space()) {
      advance();
    }
  }

  bool try_consume_char(char expected) {
    if (peek() == expected) {
      advance();
      return true;
    }
    return false;
  }

  string read_arg() {
    string arg;
    while (!at_end() && !is_space()) {
      char current = peek();
      if (current == '\\') {
        advance();
        if (!at_end()) {
          arg += advance();
        }
      } else if (current == '\'') {
        read_single_quote_section(arg);
      } else if (current == '"') {
        read_double_quote_section(arg);
      } else if (current == '$') {
        advance();
        read_variable(arg);
      } else {
        arg += advance();
      }
    }
    return arg;
  }

  void append_value(string& arg, const string& name) {
    if (auto variable = shell_variables.find(name); variable != shell_variables.end()) {
      arg += variable->second;
    } else if (const char* value = getenv(name.c_str()))
      arg += value;
  }

  void read_variable(string& arg) {
    if (try_consume_char('{')) {
      string name;
      while (!at_end() && peek() != '}')
        name += advance();
      if (!try_consume_char('}')) { /* unterminated */
      }

      append_value(arg, name);
      return;
    }

    if (!is_valid_start(peek())) {
      arg += '$';
      return;
    }

    string name;
    while (!at_end() && is_valid_name_char(peek()))
      name += advance();

    append_value(arg, name);
  }

  void read_single_quote_section(string& arg) {
    advance();
    while (!at_end() && peek() != '\'') {
      arg += advance();
    }
    try_consume_char('\'');
  }

  void read_double_quote_section(string& arg) {
    auto is_escapable = [](char c) { return c == '"' || c == '\\' || c == '$' || c == '`' || c == '\n'; };

    advance();
    while (!at_end() && peek() != '"') {
      char current = advance();
      if (current == '\\' && !at_end() && is_escapable(peek())) {
        arg += advance();
      } else if (current == '$') {
        read_variable(arg);
      } else {
        arg += current;
      }
    }
    try_consume_char('"');
  }

  string_view source_;
  ptrdiff_t position_ = 0;
};

inline vector<string> split_args(string_view input) { return ArgSplitter(input).split(); }
