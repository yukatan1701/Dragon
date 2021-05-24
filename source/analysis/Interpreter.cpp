#include "dragon/analysis/Interpreter.h"
#include <optional>

typedef Keyword::Kind Kind;

const double EPS = std::numeric_limits<double>::epsilon();

void Interpreter::processUnary(const PrefixOperator *Op, Token *Top) {
  DRAGON_DEBUG(dbgs() << "[RUNTIME] Processing unary operator `" <<
               Op->kindToString() << "` for token " << Top->toString() << ".\n");
  if (auto Id = dynamic_cast<Identifier *>(Top)) {
    auto ConstItr = mVarTableStack.front().find(Id->getName());
    if (ConstItr != mVarTableStack.front().end()) {
      Top = ConstItr->second;
    } else {
      throw InterpreterException("Variable with name `" + Id->getName() +
                                 "` does not exist in this scope at " +
                                 Op->getPos());
    }
  }
  if (auto Int = dynamic_cast<IntConstant *>(Top)) {
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
  } else if (auto Float = dynamic_cast<FloatConstant *>(Top)) {
    switch (Op->getKind()) {
    case Kind::UNARY_MINUS:
      Float->setValue(-Float->getValue());
      break;
    case Kind::PRINTLN:
      std::cout << Float->getValue() << "\n";
      break;
    case Kind::PRINT:
      std::cout << Float->getValue();
      break;
    default:
      throw InterpreterException("Unexpected unary operator for constant " +
                                 Float->getPos());
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
                                 Float->getPos());
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
      auto ConstItr = mVarTableStack.front().find(Id->getName());
      if (ConstItr != mVarTableStack.front().end())
        return ConstItr->second;
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
  IntConstant *Int1, *Int2;
  FloatConstant *F1, *F2;
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
    Int1 = dynamic_cast<IntConstant *>(OpLeft);
    Int2 = dynamic_cast<IntConstant *>(OpRight);
    if (!Int1 || !Int2)
      throw InterpreterException("Type mismatch for bitwise operation.");
    if (OpKind == Kind::BITWISE_AND)
      return new IntConstant(Int1->getValue() & Int2->getValue());
    else if (OpKind == Kind::BITWISE_OR)
      return new IntConstant(Int1->getValue() | Int2->getValue());
    else if (OpKind == Kind::SHL)
      return new IntConstant(Int1->getValue() << Int2->getValue());
    else if (OpKind == Kind::SHR)
      return new IntConstant(Int1->getValue() >> Int2->getValue());
    else if (OpKind == Kind::MODULE)
      return new IntConstant(Int1->getValue() % Int2->getValue());
    else
      return new IntConstant(Int1->getValue() ^ Int2->getValue());
    assert(0);
  default:
    Int1 = dynamic_cast<IntConstant *>(OpLeft);
    Int2 = dynamic_cast<IntConstant *>(OpRight);
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
        return new IntConstant(Int1->getValue() + Int2->getValue());
      else if (OpKind == Kind::MINUS)
        return new IntConstant(Int1->getValue() - Int2->getValue());
      else if (OpKind == Kind::MULTIPLY)
        return new IntConstant(Int1->getValue() * Int2->getValue());
      else if (OpKind == Kind::DIVIDE)
        return new FloatConstant(Int1->getValue() / double(Int2->getValue()));
      assert(0);
    }
    F1 = dynamic_cast<FloatConstant *>(OpLeft);
    F2 = dynamic_cast<FloatConstant *>(OpRight);
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
        return new FloatConstant(F1->getValue() + F2->getValue());
      else if (OpKind == Kind::MINUS)
        return new FloatConstant(F1->getValue() - F2->getValue());
      else if (OpKind == Kind::MULTIPLY)
        return new FloatConstant(F1->getValue() * F2->getValue());
      else if (OpKind == Kind::DIVIDE)
        return new FloatConstant(F1->getValue() / F2->getValue());
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
        return new FloatConstant(Int1->getValue() + F2->getValue());
      else if (OpKind == Kind::MINUS)
        return new FloatConstant(Int1->getValue() - F2->getValue());
      else if (OpKind == Kind::MULTIPLY)
        return new FloatConstant(Int1->getValue() * F2->getValue());
      else if (OpKind == Kind::DIVIDE)
        return new FloatConstant(Int1->getValue() / double(F2->getValue()));
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
        return new FloatConstant(F1->getValue() + Int2->getValue());
      else if (OpKind == Kind::MINUS)
        return new FloatConstant(F1->getValue() - Int2->getValue());
      else if (OpKind == Kind::MULTIPLY)
        return new FloatConstant(F1->getValue() * Int2->getValue());
      else if (OpKind == Kind::DIVIDE)
        return new FloatConstant(F1->getValue() / double(Int2->getValue()));
      assert(0);
    }
    throw InterpreterException("Some type mismatch expected.");
  }

}

void processBinaryGoto(Token *OpLeft, Token *OpRight, std::size_t &Idx) {
  if (auto Bool = dynamic_cast<Boolean *>(OpLeft)) {
    if (auto Int = dynamic_cast<IntConstant *>(OpRight)) {
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
    auto &VarTable = mVarTableStack.front();
    auto Itr = VarTable.find(Id->getName());
    Constant **Value = nullptr;
    if (Itr == VarTable.end()) {
      DRAGON_DEBUG(
          dbgs() << "[RUNTIME] Create new table entry for left operand.\n");
      Value = &VarTable.insert(std::make_pair(
          Id->getName(), nullptr)).first->second;
    } else {
      DRAGON_DEBUG(
          dbgs() << "[RUNTIME] Use existing table entry for left operand.\n");
      Value = &Itr->second;
    }
    if (auto ConstRight = dynamic_cast<Constant *>(OpRight)) {
      *Value = ConstRight->cloneConst();
    } else if (auto IdRight = dynamic_cast<Identifier *>(OpRight)) {
      auto RightItr = VarTable.find(IdRight->getName());
      if (RightItr == VarTable.end())
        throw InterpreterException("Failed to read variable `" + IdRight->getName() +
                                   "` at " + OpLeft->getPos());
      *Value = RightItr->second->cloneConst();
    } else {
      throw InterpreterException("Non-constant right expression at " +
                                OpLeft->getPos());
    }
    DRAGON_DEBUG(dbgs() << "[RUNTIME] New token for a value of left operand: " <<
                 (*Value)->toString() << "\n");
    mTmpTokens.top().insert(*Value);
  } else {
    throw InterpreterException("R-value error at " +
                                OpLeft->getPos());
  }
}

bool Interpreter::run(const Function &F) {
  auto getVarItr = [this](const Identifier *Id) {
    auto &VT = mVarTableStack.front();
    auto VarItr = VT.find(Id->getName());
    if (VarItr == VT.end())
      throw InterpreterException("Variable with name `" +
          Id->getName() + "` does not exist in this scope");
    return VarItr;
  };
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
            auto NextItr = std::next(Itr);
            if (NextItr == PL[Idx].end())
              throw InterpreterException("Not enough arguments");
            auto ArgToken = *NextItr;
            Constant *ConstValue = nullptr;
            if (auto Id = dynamic_cast<Identifier *>(ArgToken)) {
              auto VarItr = getVarItr(Id);
              ConstValue = VarItr->second->cloneConst();
            } else if (auto Const = dynamic_cast<Constant *>(ArgToken)) {
              ConstValue = Const->cloneConst();
            } else {
              throw InterpreterException("Invalid argument type");
            }
            mCallStack.push(ConstValue);
            ++Itr;
          }
          auto HasReturned = callFunction(FuncItr->second.getName());
          if (HasReturned) {
            auto RetConst = mCallStack.top();
            mTmpTokens.top().insert(RetConst);
            Stack.push(RetConst);
          }
        }
      } else if (auto Kw = dynamic_cast<Keyword *>(Token)) {
        if (Stack.empty() && Kw->getKind() != Kind::RETURN)
          throw InterpreterException("Unexpected unary operator at " +
                                     Kw->getPos());
        if (auto Unary = dynamic_cast<PrefixOperator *>(Kw)) {
          if (Unary->getKind() == Kind::RETURN) {
            if (Stack.empty()) {
              return false;
            } else {
              auto Top = Stack.top();
              Constant *RetConst = nullptr;
              if (auto Id = dynamic_cast<Identifier *>(Top)) {
                RetConst = getVarItr(Id)->second->cloneConst();
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
            if (auto Int = dynamic_cast<IntConstant *>(Stack.top())) {
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
            mTmpTokens.top().insert(Res);
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
  auto &CurTmp = mTmpTokens.emplace();
  for (auto Itr = ParamList.rbegin(); Itr != ParamList.rend(); ++Itr) {
    auto ParamConst = mCallStack.top()->cloneConst();
    CurTmp.insert(ParamConst);
    VarTable.insert(std::make_pair((*Itr)->getName(), ParamConst));
    delete mCallStack.top();
    mCallStack.pop();
  }
  auto HasReturned = run(Func);
  if (!mTmpTokens.empty()) {
    for (auto Ptr : mTmpTokens.top())
      delete Ptr;
    mTmpTokens.pop();
  }
  mVarTableStack.pop_front();
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
    for (auto Ptr : mTmpTokens.top())
      delete Ptr;
    mTmpTokens.pop();
  }
}