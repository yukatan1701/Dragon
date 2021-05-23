#include "dragon/analysis/Interpreter.h"
#include <iostream>
#include <fstream>

int main(int argc, char **argv) {
  std::cout << std::endl;
  if (argc < 2) {
    DRAGON_DEBUG(dbgs() << "Too few arguments.\n");
    return -1;
  }
  auto Filename = argv[1];
  std::ifstream File;
  File.open(Filename, std::ios::in);
  if (!File.is_open()) {
    DRAGON_DEBUG(dbgs() << "Failed to open file `" << Filename << "`.\n");
    return -1;
  }
  try {
    LexicalAnalyzer LA(File);
    SyntaxAnalyzer SA(LA);
    Interpreter Int(SA);
  } catch (std::exception &E) {
    DRAGON_DEBUG(dbgs() << "Exception occured:\n");
    DRAGON_DEBUG(dbgs() << E.what());
  }
  File.close();
  
  return 0;
}