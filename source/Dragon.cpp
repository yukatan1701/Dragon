#include "dragon/analysis/Interpreter.h"
#include <iostream>
#include <fstream>

#define RED_TEXT "\033[1;31m"

int main(int argc, char **argv) {
  std::cout << "DRAGON 1.0 is running." << std::endl;
  if (argc < 2) {
    std::cerr << "Too few arguments. Please enter a filename." << std::endl;
    return -1;
  }
  auto Filename = argv[1];
  std::ifstream File;
  File.open(Filename, std::ios::in);
  if (!File.is_open()) {
    std::cerr << "Failed to open file `" << Filename << "`." << std::endl;
    return -1;
  }
  try {
    LexicalAnalyzer LA(File);
    SyntaxAnalyzer SA(LA);
    Interpreter Int(SA);
  } catch (std::exception &E) {
    std::cerr << RED_TEXT << E.what();
  }
  File.close();
  return 0;
}