#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class ArgSplitter {
public:
  explicit ArgSplitter(std::string_view source) : source_(source) {}

  std::vector<std::string> split() {
    std::vector<std::string> args;
    while (!at_end()) {
      skip_whitespace();
      if (at_end())
        break;
      std::string arg = read_arg();
      if (!arg.empty()) {
        args.push_back(std::move(arg));
      }
    }
    return args;
  }

private:
  bool at_end() const { return position_ >= source_.size(); }

  char peek() const { return at_end() ? '\0' : source_[position_]; }

  bool is_space() const {
    return std::isspace(static_cast<unsigned char>(peek()));
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

  std::string read_arg() {
    std::string arg;
    while (!at_end() && !is_space()) {
      char curr = peek();
      if (curr == '\\') {
        advance();
        if (!at_end()) {
          arg += advance();
        }
      } else if (curr == '\'') {
        read_single_quote_section(arg);
      } else if (curr == '"') {
        read_double_quote_section(arg);
      } else {
        arg += advance();
      }
    }
    return arg;
  }

  void read_single_quote_section(std::string &arg) {
    advance();
    while (!at_end() && peek() != '\'') {
      arg += advance();
    }
    try_consume_char('\'');
  }

  void read_double_quote_section(std::string &arg) {

    auto is_escapable = [](char c) {
      return c == '"' || c == '\\' || c == '$' || c == '`' || c == '\n';
    };

    advance();
    while (!at_end() && peek() != '"') {
      char curr = advance();
      if (curr == '\\' && !at_end() && is_escapable(peek())) {
        arg += advance();
      } else {
        arg += curr;
      }
    }
    try_consume_char('"');
  }

  std::string_view source_;
  size_t position_ = 0;
};

inline std::vector<std::string> split_args(std::string_view input) {
  return ArgSplitter(input).split();
}