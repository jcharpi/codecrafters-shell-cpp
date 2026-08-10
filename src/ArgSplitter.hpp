#pragma once

#include <cctype>
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
      if (at_end())
        break;
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

  bool is_space() const {
    return isspace(static_cast<unsigned char>(peek()));
  }

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
      } else {
        arg += advance();
      }
    }
    return arg;
  }

  void read_single_quote_section(string &arg) {
    advance();
    while (!at_end() && peek() != '\'') {
      arg += advance();
    }
    try_consume_char('\'');
  }

  void read_double_quote_section(string &arg) {

    auto is_escapable = [](char c) {
      return c == '"' || c == '\\' || c == '$' || c == '`' || c == '\n';
    };

    advance();
    while (!at_end() && peek() != '"') {
      char current = advance();
      if (current == '\\' && !at_end() && is_escapable(peek())) {
        arg += advance();
      } else {
        arg += current;
      }
    }
    try_consume_char('"');
  }

  string_view source_;
  ptrdiff_t position_ = 0;
};

inline vector<string> split_args(string_view input) {
  return ArgSplitter(input).split();
}