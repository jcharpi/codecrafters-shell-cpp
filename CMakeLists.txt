cmake_minimum_required(VERSION 3.13)

project(shell-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

add_executable(shell ${SOURCE_FILES})

find_path(READLINE_INCLUDE_DIR
  NAMES readline/history.h
  HINTS /opt/homebrew/opt/readline/include /usr/local/opt/readline/include)
find_library(READLINE_LIBRARY
  NAMES readline
  HINTS /opt/homebrew/opt/readline/lib /usr/local/opt/readline/lib)

target_include_directories(shell PRIVATE ${READLINE_INCLUDE_DIR})
target_link_libraries(shell PRIVATE ${READLINE_LIBRARY})
