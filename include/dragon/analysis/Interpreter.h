#ifndef __DRAGON_INTERPRETER__
#define __DRAGON_INTERPRETER__

#include "dragon/analysis/SyntaxAnalyzer.h"
#include <map>
#include <set>
#include <stack>
#include <deque>

class InterpreterException : public std::exception {
public:
  InterpreterException(const std::string &Msg) {
    mMsg = "[RUNTIME EXCEPTION] " + Msg + ".\n";
  }
  virtual const char *what() const noexcept { return mMsg.c_str(); }
private:
  std::string mMsg;
};

class Interpreter {
public:
  Interpreter(const SyntaxAnalyzer &SA);
  ~Interpreter();
private:
  typedef SyntaxAnalyzer::FuncMap FuncMap;
  typedef std::map<std::string, Constant *> VarTable;
  typedef std::pair<VarTable::iterator, VarTable *> VarTableItrPair;
  const FuncMap &mFM;
  std::stack<Constant *> mCallStack;
  std::deque<std::set<Token *>> mTmpTokens; 
  std::deque<VarTable> mVarTableStack;
  std::deque<std::set<std::string>> mGlobVarSetStack;
  std::stack<const Function *> mFuncStack;

  bool callFunction(const std::string &FName);
  void processUnary(const PrefixOperator *Op, Token *Top);
  void processAssign(Token *OpLeft, Token *OpRight);
  Token *processBinary(const BinaryOperator *Op, Token *OpLeft, Token *OpRight);
  VarTableItrPair getVarItr(const Identifier *Id, bool Exception=true);
  bool run(const Function &F);
};

#endif