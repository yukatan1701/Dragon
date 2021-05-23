#ifndef __DRAGON_SYNTAX_ANALYZER__
#define __DRAGON_SYNTAX_ANALYZER__

#include "dragon/analysis/LexicalAnalyzer.h"

typedef std::vector<std::vector<Token *>> PostfixList;

class Function {
public:
  Function(const std::string &Name) : mName(Name) {}
  std::string getName() const { return mName; }
  void addParam(Identifier *Param) { mParams.push_back(Param); }
  const std::vector<Identifier *> &getParamList() const { return mParams; }
  PostfixList &getPostfixList() { return mPL; }
  const PostfixList &getPostfixList() const { return mPL; }
private:
  std::string mName;
  std::vector<Identifier *> mParams;
  PostfixList mPL;
};

class SyntaxException : public std::exception {
public:
  SyntaxException(const std::string &Msg) {
    mMsg = "[SYNTAX EXCEPTION] " + Msg + ".\n";
  }
  virtual const char *what() const noexcept { return mMsg.c_str(); }
private:
  std::string mMsg;
};

class SyntaxAnalyzer {
  typedef LexicalAnalyzer::TokenList TokenList;
  typedef std::vector<std::vector<Token *>> TokenPtrList;
public:
  typedef std::map<std::string, Function> FuncMap;
  SyntaxAnalyzer(const LexicalAnalyzer &LA);
  const FuncMap &getFuncMap() const { return mFuncMap; }
  void dump() const;
private:
  void generatePostfix(const TokenPtrList &TL, Function &F);
  FuncMap mFuncMap;
  std::vector<std::unique_ptr<Token>> mTmpTokens;
};

#endif