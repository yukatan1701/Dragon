cmake_minimum_required(VERSION 3.0)

project(Dragon VERSION 1.0 LANGUAGES CXX)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS "-Wall")

include_directories(include)

file(GLOB_RECURSE ALL_SOURCES
  source/analysis/Token.cpp
  source/analysis/LexicalAnalyzer.cpp
  source/analysis/SyntaxAnalyzer.cpp
  source/analysis/Interpreter.cpp
  source/Dragon.cpp
)

add_executable(dragon ${ALL_SOURCES})


