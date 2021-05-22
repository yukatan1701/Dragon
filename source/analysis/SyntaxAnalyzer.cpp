#include "dragon/analysis/SyntaxAnalyzer.h"
#include <stack>

typedef LexicalAnalyzer::TokenList TokenList;
typedef SyntaxAnalyzer::FuncMap FuncMap;

void SyntaxAnalyzer::generatePostfix(const TokenList &TL,
                     const TokenList::const_iterator &ItrBegin,
                     const TokenList::const_iterator &ItrEnd,
                     Function &F) {
  typedef std::pair<Keyword *, std::size_t> IfWhilePos;
  // TODO: square brackets
  auto isFunction = [this](const Identifier *Id) {
    return mFuncMap.find(Id->getName()) != mFuncMap.end();
  };
  auto generateNotGoto = [this](const PostfixList &PL)->
      std::vector<Token *> {
    auto NotPtr = mTmpTokens.emplace_back(
        std::make_unique<Keyword>(Keyword::Kind::LOGICAL_NOT)).get();
    auto GotoPtr = mTmpTokens.emplace_back(
        std::make_unique<Keyword>(Keyword::Kind::GOTO_BIN)).get();
    auto PosPtr = mTmpTokens.emplace_back(
        std::make_unique<IntConstant>(PL.size())).get();
    return { NotPtr, PosPtr, GotoPtr };
  };
  std::stack<IfWhilePos> IfWhileStack;
  auto &PostfixList = F.getPostfixList();
  for (auto Itr = ItrBegin; Itr != ItrEnd; ++Itr) {
    auto &Line = PostfixList.emplace_back();
    std::stack<Token *> Stack;
    for (auto &Token : *Itr) {
      auto TokenPtr = Token.get();
      if (dynamic_cast<Constant *>(TokenPtr) ||
          dynamic_cast<String *>(TokenPtr)) {
        Line.push_back(TokenPtr);
      } else if (auto Id = dynamic_cast<Identifier *>(TokenPtr)) {
        if (isFunction(Id))
          Stack.push(Id);
        else
          Line.push_back(Id);
      } else if (auto Pref = dynamic_cast<PrefixOperator *>(TokenPtr)) {
        if (Pref->getKind() == Keyword::Kind::COMMA) {
          bool HasLeftBr = false;
          while (!Stack.empty()) {
            if (auto Kw = dynamic_cast<Bracket *>(Stack.top());
                Kw->getKind() == Keyword::Kind::LEFT_PARENTHESIS) {
              HasLeftBr = true;
              break;
            }
            Line.push_back(Stack.top());
            Stack.pop();
          }
          if (!HasLeftBr)
            throw SyntaxException("Bracket mismatch or missed comma at " +
                                  Pref->getPos());
        } else if (Pref->getKind() == Keyword::Kind::IF ||
                   Pref->getKind() == Keyword::Kind::WHILE) {
          IfWhileStack.push(std::make_pair(Pref, PostfixList.size() - 1));
        } else if (Pref->getKind() == Keyword::Kind::ELSE) {
          if (IfWhileStack.empty())
            throw SyntaxException("No `if` for `else` at " + Pref->getPos());
          auto If = IfWhileStack.top().first;
          auto IfPos = IfWhileStack.top().second;
          if (If->getKind() != Keyword::Kind::IF)
            throw SyntaxException("No `if` for `else` at " + Pref->getPos());
          auto NotGotoList = generateNotGoto(PostfixList);
          PostfixList[IfPos].insert(PostfixList[IfPos].end(),
                                    NotGotoList.begin(), NotGotoList.end());
          IfWhileStack.pop();
          IfWhileStack.push(std::make_pair(Pref, PostfixList.size() - 1));
        } else if (Pref->getKind() == Keyword::Kind::ENDIF) {
          if (IfWhileStack.empty())
            throw SyntaxException("No `if` for `endif` at " + Pref->getPos());
          auto If = IfWhileStack.top().first;
          auto IfPos = IfWhileStack.top().second;
          if (If->getKind() != Keyword::Kind::IF &&
              If->getKind() != Keyword::Kind::ELSE)
            throw SyntaxException("No `if` or `else` for `endif` at " + Pref->getPos());
          if (If->getKind() == Keyword::Kind::IF) {
            auto NotGotoList = generateNotGoto(PostfixList);
            PostfixList[IfPos].insert(PostfixList[IfPos].end(),
                                      NotGotoList.begin(), NotGotoList.end());
          }
          IfWhileStack.pop();
        } else if (Pref->getKind() == Keyword::Kind::ENDWHILE) {
          if (IfWhileStack.empty())
            throw SyntaxException("No `while` for `endwhile` at " + Pref->getPos());
          auto While = IfWhileStack.top().first;
          auto WhilePos = IfWhileStack.top().second;
          if (While->getKind() != Keyword::Kind::WHILE)
            throw SyntaxException("No `while` for `endwhile` at " + Pref->getPos());
          auto NotGotoList = generateNotGoto(PostfixList);
          PostfixList[WhilePos].insert(PostfixList[WhilePos].end(),
                                       NotGotoList.begin(), NotGotoList.end());
          auto GotoPtr = mTmpTokens.emplace_back(
              std::make_unique<Keyword>(Keyword::Kind::GOTO_UN)).get();
          auto PosPtr = mTmpTokens.emplace_back(
              std::make_unique<IntConstant>(WhilePos)).get();
          Line.push_back(PosPtr);
          Line.push_back(GotoPtr);
          IfWhileStack.pop();
        } else {
          Stack.push(Pref);
        }
      } else if (auto Br = dynamic_cast<Bracket *>(TokenPtr)) {
        if (Br->getKind() == Keyword::Kind::LEFT_PARENTHESIS ||
            Br->getKind() == Keyword::Kind::LEFT_SQUARE) {
          Stack.push(Br);
        } else if (Br->getKind() == Keyword::Kind::RIGHT_PARENTHESIS) {
          if (Stack.empty())
            throw SyntaxException("Bracket mismatch at " + Br->getPos());
          while (!Stack.empty()) {
            auto TopToken = dynamic_cast<Bracket *>(Stack.top());
            if (TopToken &&
                TopToken->getKind() == Keyword::Kind::LEFT_PARENTHESIS)
              break;
            Line.push_back(Stack.top());
            Stack.pop();
          }
          if (!Stack.empty()) {
            auto TopToken = dynamic_cast<Bracket *>(Stack.top());
            if (!TopToken ||
                TopToken->getKind() != Keyword::Kind::LEFT_PARENTHESIS)
              throw SyntaxException("Bracket mismatch at " + Br->getPos());
            else
              Stack.pop();
            auto Id = dynamic_cast<Identifier *>(Stack.top());
            if (Id && isFunction(Id)) {
              Line.push_back(Stack.top());
              Stack.pop();
            }
          }
        }
      } else if (auto Bin = dynamic_cast<BinaryOperator *>(TokenPtr)) {
        while (!Stack.empty()) {
          auto Kw = dynamic_cast<Keyword *>(Stack.top());
          assert(Kw && "Stack must contain keyword only!");
          if (dynamic_cast<Bracket *>(Kw))
            break;
          if (Kw->getPriority() < Bin->getPriority()) {
            Line.push_back(Kw);
            Stack.pop();
          } else if (Kw->getPriority() == Bin->getPriority()) {
            auto BinKw = dynamic_cast<BinaryOperator *>(Kw);
            if (!BinKw)
              break;
            if (BinKw->getAssocKind() == BinaryOperator::AssocKind::LEFT) {
              Line.push_back(BinKw);
              Stack.pop();
            } else {
              break;
            }
          } else {
            break;
          }
        }
        Stack.push(Bin);
      } else {
        throw SyntaxException("Unexpected token at " + TokenPtr->getPos());
      }
    }
    while (!Stack.empty()) {
      if (!dynamic_cast<Keyword *>(Stack.top())) {
        throw SyntaxException("Parenthesis mismatch");
      }
      Line.push_back(Stack.top());
      Stack.pop();
    }
  }
  if (!IfWhileStack.empty()) {
    auto TopToken = IfWhileStack.top().first;
    throw SyntaxException("Pair mismatch for token at " + TopToken->getPos());
  }
  PostfixList.emplace_back();
}

SyntaxAnalyzer::SyntaxAnalyzer(const LexicalAnalyzer &LA) {
  auto &TokenList = LA.getTokenList();
  auto getReturnIterator = [&TokenList](TokenList::const_iterator Itr) {
    for (; Itr != TokenList.end(); ++Itr) {
      if (Itr->empty())
        continue;
      auto Kw = dynamic_cast<Keyword *>((*Itr)[0].get());
      if (!Kw)
        continue;
      if (Kw->getKind() == Keyword::Kind::FUNCTION)
        return TokenList.end();
      if (Kw->getKind() == Keyword::Kind::RETURN)
        break;
    }
    return Itr;
  };
  for (auto Itr = TokenList.begin(); Itr != TokenList.end(); ++Itr) {
    auto &TokenLine = *Itr;
    if (TokenLine.empty())
      continue;
    if (auto Kw = dynamic_cast<Keyword *>(TokenLine[0].get())) {
      if (Kw->getKind() == Keyword::Kind::FUNCTION) {
        if (TokenLine.size() < 2) {
          throw SyntaxException("Function name expected after token at " +
                                Kw->getPos());
        }
        if (auto Name = dynamic_cast<Identifier *>(TokenLine[1].get())) {
          Function Func(Name->getName());
          assert(!Func.getName().empty() && "Function name must not be empty!");
          if (TokenLine.size() < 3) {
            throw SyntaxException("'(' expected after token at " + Name->getPos());
          }
          if (auto KwLeftPar = dynamic_cast<Keyword *>(TokenLine[2].get());
              KwLeftPar->getKind() == Keyword::Kind::LEFT_PARENTHESIS) {
            if (TokenLine.size() < 4) {
              throw SyntaxException("'(' or parameter expected after token at " +
                                    KwLeftPar->getPos());
            }
            for (std::size_t I = 3; I < TokenLine.size(); I += 2) {
              auto Token = TokenLine[I].get();
              if (auto ParamId = dynamic_cast<Identifier *>(Token)) {
                Func.addParam(ParamId);
                if (I + 1 == TokenLine.size())
                  throw SyntaxException("'(' expected after token at " +
                                        ParamId->getPos());
                auto KwSep = dynamic_cast<Keyword *>(TokenLine[I + 1].get());
                if (KwSep && KwSep->getKind() ==
                    Keyword::Kind::RIGHT_PARENTHESIS) {
                  // If ')' is the last token.
                  if (I + 2 != TokenLine.size())
                    throw SyntaxException(
                        "Extra tokens after ')' in the function declaration at" +
                        KwSep->getPos());
                  break;
                } else if (KwSep && KwSep->getKind() == Keyword::Kind::COMMA) {
                  continue;
                } else {
                  assert(0 && "Unreachable line detected.");
                }
                assert(0 && "Unreachable line detected.");
              } else if (auto KwSep = dynamic_cast<Keyword *>(
                         TokenLine[I].get()); KwSep->getKind() ==
                         Keyword::Kind::RIGHT_PARENTHESIS) {
                if (I + 1 != TokenLine.size())
                  throw SyntaxException(
                      "Extra tokens after ')' in the function declaration at" +
                      KwSep->getPos());
              } else {
                throw SyntaxException("Identifier expected after token at " + Token->getPos());
              }
            }
            auto ReturnItr = getReturnIterator(std::next(Itr));
            if (ReturnItr == TokenList.end()) {
              throw SyntaxException("Return statement for function `" +
                                    Func.getName() + "` declared on " +
                                    Kw->getPos() + " not found");
            }
            generatePostfix(TokenList, std::next(Itr), std::next(ReturnItr), Func);
            mFuncMap.insert(std::make_pair(Func.getName(), std::move(Func)));
          } else {
            throw SyntaxException("'(' expected after token at " +
                                  Name->getPos());
          }
        } else {
          throw SyntaxException("Function name expected after token at " +
                                Kw->getPos());
        }
      } else {
        // check keywords out of function
      }
    } else {
      // global variable
    }
  }
  dump();
  // create function tables
  // (function_name) -> (arguments, postfix)
  // create postfix for every table
}

void SyntaxAnalyzer::dump() const {
  dbgs() << "[SYNTAX ANALYZER] Postfix form for functions:\n";
  for (auto &Pair : mFuncMap) {
    dbgs() << "Function `" << Pair.first << "`:\n";
    dbgs() << "Parameters: (";
    const auto &ParamList = Pair.second.getParamList();
    for (auto Itr = ParamList.begin(); Itr != ParamList.end(); ++Itr) {
      dbgs() << (*Itr)->getName();
      if (std::next(Itr) != ParamList.end())
        dbgs() << ", ";
    }
    dbgs() << ")\n";
    dbgs() << "Postfix:\n";
    std::size_t LineN = 0;
    auto &PL = Pair.second.getPostfixList();
    auto MaxNumLength = std::to_string(PL.size()).size();
    for (const auto &PostfixLine : PL) {
      auto NumLength = std::to_string(LineN).size();
      std::string Space(MaxNumLength - NumLength, ' ');
      dbgs() << Space << LineN << "| ";
      for (auto Token : PostfixLine) {
        dbgs() << Token->toString() << " ";
      }
      dbgs() << "\n";
      ++LineN;
    }
    dbgs() << "\n";
  }
  dbgs() << "[SYNTAX ANALYZER] End printing postfix.\n";
}