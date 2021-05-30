#include "dragon/analysis/Interpreter.h"
#include <optional>

typedef Keyword::Kind Kind;

const double EPS = std::numeric_limits<double>::epsilon();

Interpreter::VarTableItrPair Interpreter::getVarItr(
      const Identifier *Id, bool Exception) {
  auto &GlobVars = mGlobVarSetStack.front();
  if (GlobVars.find(Id->getName()) != GlobVars.end() &&
      mFuncStack.top()->getName() != GLOBAL_FUNC) {
    auto &GlobVT = mVarTableStack.back();
    auto GlobItr = GlobVT.find(Id->getName());
    assert(GlobItr != GlobVT.end() &&
        "Global variable must have an entry in a global table!");
    return std::make_pair(GlobItr, &GlobVT);
  }
  auto &VT = mVarTableStack.front();
  auto VarItr = VT.find(Id->getName());
  if (VarItr == VT.end() && Exception)
    throw InterpreterException("Variable with name `" +
        Id->getName() + "` does not exist in this scope");
  return std::make_pair(VarItr, &VT);
}

void Interpreter::processUnary(const PrefixOperator *Op, Token *Top) {
  DRAGON_DEBUG(dbgs() << "[RUNTIME] Processing unary operator `" <<
               Op->kindToString() << "` for token " << Top->toString() << ".\n");
  if (auto Id = dynamic_cast<Identifier *>(Top)) {
    auto ConstItrPair = getVarItr(Id);
    if (ConstItrPair.first != ConstItrPair.second->end()) {
      Top = ConstItrPair.first->second;
    } else {
      throw InterpreterException("Variable with name `" + Id->getName() +
                                 "` does not exist in this scope at " +
                                 Op->getPos());
    }
  }
  if (auto Int = dynamic_cast<Integer *>(Top)) {
    switch (Op->getKind()) {
    case Kind::UNARY_MINUS:
      Int->setValue(-Int->getValue());
      break;
    case Kind::PRINTLN:
      std::cout << Int->getValue() << "\n";
      break;
    case Kind::PRINT:
      std::cout << Int->getValue();
      break;
    default:
      throw InterpreterException("Unexpected unary operator for constant " +
                                 Int->getPos());
    }
  } else if (auto FloatPtr = dynamic_cast<Float *>(Top)) {
    switch (Op->getKind()) {
    case Kind::UNARY_MINUS:
      FloatPtr->setValue(-FloatPtr->getValue());
      break;
    case Kind::PRINTLN:
      std::cout << FloatPtr->getValue() << "\n";
      break;
    case Kind::PRINT:
      std::cout << FloatPtr->getValue();
      break;
    default:
      throw InterpreterException("Unexpected unary operator for constant " +
                                 FloatPtr->getPos());
    }
  } else if (auto Str = dynamic_cast<String *>(Top)) {
    switch (Op->getKind()) {
    case Kind::PRINTLN:
      std::cout << Str->getValue() << "\n";
      break;
    case Kind::PRINT:
      std::cout << Str->getValue();
      break;
    default:
      throw InterpreterException("Unexpected unary operator for literal " +
                                 FloatPtr->getPos());
    }
  } else if (auto Bool = dynamic_cast<Boolean *>(Top)) {
    switch (Op->getKind()) {
    case Kind::PRINTLN:
      std::cout << (Bool->getValue() ? "true" : "false")  << "\n";
      break;
    case Kind::PRINT:
      std::cout << (Bool->getValue() ? "true" : "false");
      break;
    case Kind::LOGICAL_NOT:
      Bool->setValue(!Bool->getValue());
      break;
    default:
      throw InterpreterException("Unexpected unary operator for boolean " +
                                 Bool->getPos());
    }
  } else {
    throw InterpreterException("Unexpected operand type for unary operator " +
                                Op->getPos());
  }
}

Token *Interpreter::processBinary(
    const BinaryOperator *Op, Token *OpLeftToken, Token *OpRightToken) {
  DRAGON_DEBUG(dbgs() << "[RUNTIME] Processing binary operation `" <<
               Op->kindToString() << "` for tokens: " <<
               OpLeftToken->toString() << ", " <<
               OpRightToken->toString() << ".\n");
  auto CheckIdentifier = [this, Op](Token *Operand)->Constant * {
    if (auto Id = dynamic_cast<Identifier *>(Operand)) {
      auto ConstItrPair = getVarItr(Id);
      if (ConstItrPair.first != ConstItrPair.second->end())
        return ConstItrPair.first->second;
      throw InterpreterException("Variable with name `" + Id->getName() +
                                  "` does not exist in this scope at " +
                                  Op->getPos());
    } else if (auto Const = dynamic_cast<Constant *>(Operand)) {
      return Const;
    }
    throw InterpreterException("Invalid operands for binary operation " +
                               Op->getPos());
  };
  Constant *OpLeft = CheckIdentifier(OpLeftToken);
  Constant *OpRight = CheckIdentifier(OpRightToken);
  DRAGON_DEBUG(dbgs() << "[RUNTIME] Got constant values: " <<
               OpLeft->toString() << " " << OpRight->toString() << "\n");
  Integer *Int1, *Int2;
  Float *F1, *F2;
  Boolean *B1, *B2;
  String *S1, *S2;
  bool Res;
  auto OpKind = Op->getKind();

  switch (Op->getKind()) {
  case Kind::LOGICAL_AND:
  case Kind::LOGICAL_OR:
    B1 = dynamic_cast<Boolean *>(OpLeft);
    B2 = dynamic_cast<Boolean *>(OpRight);
    if (!B1 || !B2)
      throw InterpreterException("Type mismatch for logical or.");
    Res = Op->getKind() == Kind::LOGICAL_OR ?
        (B1->getValue() || B2->getValue()) :
        (B1->getValue() && B2->getValue());
    return new Boolean(Res);
  case Kind::BITWISE_AND:
  case Kind::BITWISE_OR:
  case Kind::BITWISE_XOR:
  case Kind::SHL:
  case Kind::SHR:
  case Kind::MODULE:
    Int1 = dynamic_cast<Integer *>(OpLeft);
    Int2 = dynamic_cast<Integer *>(OpRight);
    if (!Int1 || !Int2)
      throw InterpreterException("Type mismatch for bitwise operation.");
    if (OpKind == Kind::BITWISE_AND)
      return new Integer(Int1->getValue() & Int2->getValue());
    else if (OpKind == Kind::BITWISE_OR)
      return new Integer(Int1->getValue() | Int2->getValue());
    else if (OpKind == Kind::SHL)
      return new Integer(Int1->getValue() << Int2->getValue());
    else if (OpKind == Kind::SHR)
      return new Integer(Int1->getValue() >> Int2->getValue());
    else if (OpKind == Kind::MODULE)
      return new Integer(Int1->getValue() % Int2->getValue());
    else
      return new Integer(Int1->getValue() ^ Int2->getValue());
    assert(0);
  default:
    Int1 = dynamic_cast<Integer *>(OpLeft);
    Int2 = dynamic_cast<Integer *>(OpRight);
    if (Int1 && Int2) {
      if (OpKind == Kind::EQUAL)
        return new Boolean(Int1->getValue() == Int2->getValue());
      else if (OpKind == Kind::NOT_EQUAL)
        return new Boolean(Int1->getValue() != Int2->getValue());
      else if (OpKind == Kind::LESS)
        return new Boolean(Int1->getValue() < Int2->getValue());
      else if (OpKind == Kind::LEQ)
        return new Boolean(Int1->getValue() <= Int2->getValue());
      else if (OpKind == Kind::GREATER)
        return new Boolean(Int1->getValue() > Int2->getValue());
      else if (OpKind == Kind::GEQ)
        return new Boolean(Int1->getValue() >= Int2->getValue());
      else if (OpKind == Kind::PLUS)
        return new Integer(Int1->getValue() + Int2->getValue());
      else if (OpKind == Kind::MINUS)
        return new Integer(Int1->getValue() - Int2->getValue());
      else if (OpKind == Kind::MULTIPLY)
        return new Integer(Int1->getValue() * Int2->getValue());
      else if (OpKind == Kind::DIVIDE)
        return new Float(Int1->getValue() / double(Int2->getValue()));
      assert(0);
    }
    F1 = dynamic_cast<Float *>(OpLeft);
    F2 = dynamic_cast<Float *>(OpRight);
    if (F1 && F2) {
      if (OpKind == Kind::EQUAL)
        return new Boolean(F1->getValue() == F2->getValue());
      else if (OpKind == Kind::NOT_EQUAL)
        return new Boolean(F1->getValue() != F2->getValue());
      else if (OpKind == Kind::LESS)
        return new Boolean(F1->getValue() < F2->getValue());
      else if (OpKind == Kind::LEQ)
        return new Boolean(F1->getValue() <= F2->getValue());
      else if (OpKind == Kind::GREATER)
        return new Boolean(F1->getValue() > F2->getValue());
      else if (OpKind == Kind::GEQ)
        return new Boolean(F1->getValue() >= F2->getValue());
      else if (OpKind == Kind::PLUS)
        return new Float(F1->getValue() + F2->getValue());
      else if (OpKind == Kind::MINUS)
        return new Float(F1->getValue() - F2->getValue());
      else if (OpKind == Kind::MULTIPLY)
        return new Float(F1->getValue() * F2->getValue());
      else if (OpKind == Kind::DIVIDE)
        return new Float(F1->getValue() / F2->getValue());
      assert(0);
    }
    S1 = dynamic_cast<String *>(OpLeft);
    S2 = dynamic_cast<String *>(OpRight);
    if (S1 && S2) {
      if (OpKind == Kind::EQUAL)
        return new Boolean(S1->getValue() == S2->getValue());
      else if (OpKind == Kind::NOT_EQUAL)
        return new Boolean(S1->getValue() != S2->getValue());
      else if (OpKind == Kind::PLUS)
        return new String(S1->getValue() + S2->getValue());
      else
        throw InterpreterException("It is forbidden to compare strings");
    }
    B1 = dynamic_cast<Boolean *>(OpLeft);
    B2 = dynamic_cast<Boolean *>(OpRight);
    if (B1 && B2) {
      bool Res;
      if (OpKind == Kind::EQUAL)
        Res = S1->getValue() == S2->getValue();
      else if (OpKind == Kind::NOT_EQUAL)
        Res = S1->getValue() != S2->getValue();
      else
        throw InterpreterException("It is forbidden to compare bools");
      return new Boolean(Res);
    }
    if (Int1 && F2) {
      if (OpKind == Kind::EQUAL)
        return new Boolean(Int1->getValue() == F2->getValue());
      else if (OpKind == Kind::NOT_EQUAL)
        return new Boolean(Int1->getValue() != F2->getValue());
      else if (OpKind == Kind::LESS)
        return new Boolean(Int1->getValue() < F2->getValue());
      else if (OpKind == Kind::LEQ)
        return new Boolean(Int1->getValue() <= F2->getValue());
      else if (OpKind == Kind::GREATER)
        return new Boolean(Int1->getValue() > F2->getValue());
      else if (OpKind == Kind::GEQ)
        return new Boolean(Int1->getValue() >= F2->getValue());
      else if (OpKind == Kind::PLUS)
        return new Float(Int1->getValue() + F2->getValue());
      else if (OpKind == Kind::MINUS)
        return new Float(Int1->getValue() - F2->getValue());
      else if (OpKind == Kind::MULTIPLY)
        return new Float(Int1->getValue() * F2->getValue());
      else if (OpKind == Kind::DIVIDE)
        return new Float(Int1->getValue() / double(F2->getValue()));
      assert(0);
    }
    if (F1 && Int2) {
      if (OpKind == Kind::EQUAL)
        return new Boolean(F1->getValue() == Int2->getValue());
      else if (OpKind == Kind::NOT_EQUAL)
        return new Boolean(F1->getValue() != Int2->getValue());
      else if (OpKind == Kind::LESS)
        return new Boolean(F1->getValue() < Int2->getValue());
      else if (OpKind == Kind::LEQ)
        return new Boolean(F1->getValue() <= Int2->getValue());
      else if (OpKind == Kind::GREATER)
        return new Boolean(F1->getValue() > Int2->getValue());
      else if (OpKind == Kind::GEQ)
        return new Boolean(F1->getValue() >= Int2->getValue());
      else if (OpKind == Kind::PLUS)
        return new Float(F1->getValue() + Int2->getValue());
      else if (OpKind == Kind::MINUS)
        return new Float(F1->getValue() - Int2->getValue());
      else if (OpKind == Kind::MULTIPLY)
        return new Float(F1->getValue() * Int2->getValue());
      else if (OpKind == Kind::DIVIDE)
        return new Float(F1->getValue() / double(Int2->getValue()));
      assert(0);
    }
    throw InterpreterException("Some type mismatch expected.");
  }

}

void processBinaryGoto(Token *OpLeft, Token *OpRight, std::size_t &Idx) {
  if (auto Bool = dynamic_cast<Boolean *>(OpLeft)) {
    if (auto Int = dynamic_cast<Integer *>(OpRight)) {
      if (Bool->getValue()) {
        Idx = Int->getValue() - 1;
      }
    } else {
      throw InterpreterException("Integer position expected for goto at " +
                                OpLeft->getPos());
    }
  } else {
    throw InterpreterException("Boolean expected for goto at " +
                                OpLeft->getPos());
  }
}

void Interpreter::processAssign(Token *OpLeft, Token *OpRight) {
  DRAGON_DEBUG(dbgs() << "[RUNTIME] Processing assignment for tokens: " <<
               OpLeft->toString() << ", " << OpRight->toString() << ".\n");
  if (auto Id = dynamic_cast<Identifier *>(OpLeft)) {
    auto ItrPair = getVarItr(Id, false);
    Constant **Value = nullptr;
    if (ItrPair.first == ItrPair.second->end()) {
      DRAGON_DEBUG(
          dbgs() << "[RUNTIME] Create new table entry for left operand.\n");
      Value = &ItrPair.second->insert(std::make_pair(
          Id->getName(), nullptr)).first->second;
    } else {
      DRAGON_DEBUG(
          dbgs() << "[RUNTIME] Use existing table entry for left operand.\n");
      Value = &ItrPair.first->second;
    }
    if (auto ConstRight = dynamic_cast<Constant *>(OpRight)) {
      *Value = ConstRight->cloneConst();
    } else if (auto IdRight = dynamic_cast<Identifier *>(OpRight)) {
      auto RightItr = getVarItr(IdRight);
      if (RightItr.first == RightItr.second->end())
        throw InterpreterException("Failed to read variable `" + IdRight->getName() +
                                   "` at " + OpLeft->getPos());
      *Value = RightItr.first->second->cloneConst();
    } else {
      throw InterpreterException("Non-constant right expression at " +
                                OpLeft->getPos());
    }
    DRAGON_DEBUG(dbgs() << "[RUNTIME] New token for a value of left operand: " <<
                 (*Value)->toString() << "\n");
    if (ItrPair.second == &mVarTableStack.back())
      mTmpTokens.back().insert(*Value);
    else
      mTmpTokens.front().insert(*Value);
  } else {
    throw InterpreterException("R-value error at " +
                                OpLeft->getPos());
  }
}

bool Interpreter::run(const Function &F) {
  auto &PL = F.getPostfixList();
  for (std::size_t Idx = 0; Idx < PL.size(); ++Idx) {
    std::stack<Token *> Stack;
    for (auto Itr = PL[Idx].begin(); Itr != PL[Idx].end(); ++Itr) {
      DRAGON_DEBUG(dbgs() << "[RUNTIME] Checking token " << (*Itr)->toString()
                   << ".\n");
      //DRAGON_DEBUG(dbgs() << "[RUNTIME] Is Kw: " << dynamic_cast<Keyword *>(*Itr) << "\n");
      auto Token = *Itr;
      if (dynamic_cast<Constant *>(Token)) {
        Stack.push(Token);
      } else if (auto Id = dynamic_cast<Identifier *>(Token)) {
        auto FuncItr = mFM.find(Id->getName());
        if (FuncItr == mFM.end()) {
          Stack.push(Token);
        } else {
          auto ParamCount = FuncItr->second.getParamList().size();
          for (std::size_t I = 0; I < ParamCount; ++I) {
            if (Stack.empty())
              throw InterpreterException("Not enough arguments");
            auto ArgToken = Stack.top();
            Constant *ConstValue = nullptr;
            if (auto Id = dynamic_cast<Identifier *>(ArgToken)) {
              auto VarItr = getVarItr(Id);
              DRAGON_DEBUG(dbgs() << "[RUNTIME] Add variable to call stack: "
                           << VarItr.first->second->toString() << "\n");
              ConstValue = VarItr.first->second->cloneConst();
            } else if (auto Const = dynamic_cast<Constant *>(ArgToken)) {
              ConstValue = Const->cloneConst();
            } else {
              throw InterpreterException("Invalid argument type");
            }
            mCallStack.push(ConstValue);
            Stack.pop();
          }
          auto HasReturned = callFunction(FuncItr->second.getName());
          if (HasReturned) {
            auto RetConst = mCallStack.top();
            mTmpTokens.front().insert(RetConst);
            Stack.push(RetConst);
          }
        }
      } else if (auto Kw = dynamic_cast<Keyword *>(Token)) {
        if (Stack.empty() && Kw->getKind() != Kind::RETURN)
          throw InterpreterException("Unexpected unary operator at " +
                                     Kw->getPos());
        if (auto Unary = dynamic_cast<PrefixOperator *>(Kw)) {
          if (Unary->getKind() == Kind::GLOBAL) {
            if (Stack.empty())
              throw InterpreterException("Not enough arguments for `global` at "
                                         + Unary->getPos());
            auto Id = dynamic_cast<Identifier *>(Stack.top());
            if (!Id)
              throw InterpreterException("Not an identifier after `global` at "
                                         + Unary->getPos());
            auto &GlobalTable = mVarTableStack.back();
            auto GlobItr = GlobalTable.find(Id->getName());
            if (GlobItr == GlobalTable.end())
              throw InterpreterException("Failed to find global variable at "
                                         + Unary->getPos());
            mGlobVarSetStack.front().insert(Id->getName());
            Stack.pop();
            continue;
          }
          if (Unary->getKind() == Kind::RETURN) {
            if (Stack.empty()) {
              return false;
            } else {
              auto Top = Stack.top();
              Constant *RetConst = nullptr;
              if (auto Id = dynamic_cast<Identifier *>(Top)) {
                RetConst = getVarItr(Id).first->second->cloneConst();
              } else if (auto Const = dynamic_cast<Constant *>(Top)) {
                RetConst = Const->cloneConst();
              } else {
                throw InterpreterException("Unexpected kind of returning value at " +
                                     Kw->getPos());
              }
              mCallStack.push(RetConst);
              return true;
            }
          } else if (Unary->getKind() == Kind::GOTO_UN) {
            if (auto Int = dynamic_cast<Integer *>(Stack.top())) {
              Idx = Int->getValue() - 1;
            } else {
              throw InterpreterException("Non-integer goto found");
            }
            break;
          } else {
            processUnary(Unary, Stack.top());
          }
        } else if (auto Binary = dynamic_cast<BinaryOperator *>(Kw)) {
          if (Stack.size() < 2)
            throw InterpreterException("Not enough operands at " + Kw->getPos());
          auto OpRight = Stack.top();
          Stack.pop();
          auto OpLeft = Stack.top();
          if (Binary->getKind() == Kind::GOTO_BIN) {
            processBinaryGoto(OpLeft, OpRight, Idx);
            Stack.pop();
            break;
          } else if (Binary->getKind() == Kind::ASSIGN) {
            processAssign(OpLeft, OpRight);
          } else {
            auto Res = processBinary(Binary, OpLeft, OpRight);
            Stack.pop();
            mTmpTokens.front().insert(Res);
            Stack.push(Res);
          }
        } else {
          throw InterpreterException("Unexpected keyword `" +
              Kw->kindToString() + "` at " + Kw->getPos());
        }
      } else {
        throw InterpreterException("Unexpected token at " + Token->getPos());
      }
    }
  }
  return false;
}

bool Interpreter::callFunction(const std::string &FName) {
  DRAGON_DEBUG(dbgs() << "[RUNTIME] Entering function `" << FName << "`.\n");
  auto Itr = mFM.find(FName);
  if (Itr == mFM.end())
    throw InterpreterException("Function with name `" + FName +
                               "` does not exist");
  auto &Func = Itr->second;
  auto &ParamList = Func.getParamList();
  if (ParamList.size() > mCallStack.size())
    throw InterpreterException("Not enough arguments for function `" + FName +
                               "`.");
  auto &VarTable = mVarTableStack.emplace_front();
  auto &CurTmp = mTmpTokens.emplace_front();
  mGlobVarSetStack.emplace_front();
  for (auto Itr = ParamList.rbegin(); Itr != ParamList.rend(); ++Itr) {
    auto ParamConst = mCallStack.top()->cloneConst();
    CurTmp.insert(ParamConst);
    VarTable.insert(std::make_pair((*Itr)->getName(), ParamConst));
    delete mCallStack.top();
    mCallStack.pop();
  }
  mFuncStack.push(&Func);
  auto HasReturned = run(Func);
  if (FName != GLOBAL_FUNC) {
    if (!mTmpTokens.empty()) {
      for (auto Ptr : mTmpTokens.front())
        delete Ptr;
      mTmpTokens.pop_front();
    }
    mVarTableStack.pop_front();
    mGlobVarSetStack.pop_front();
  }
  mFuncStack.pop();
  DRAGON_DEBUG(dbgs() << "[RUNTIME] Leaving function `" << FName << "`.\n");
  return HasReturned;
}

Interpreter::Interpreter(const SyntaxAnalyzer &SA) : mFM(SA.getFuncMap()) {
  mCallStack.emplace();
  callFunction(GLOBAL_FUNC);
  if (mFM.find("main") != mFM.end()) {
    callFunction("main");
  }
};

Interpreter::~Interpreter() {
  while (!mTmpTokens.empty()) {
    for (auto Ptr : mTmpTokens.front())
      delete Ptr;
    mTmpTokens.pop_front();
  }
}