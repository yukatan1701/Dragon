#ifndef __DRAGON_TOKEN__
#define __DRAGON_TOKEN__

#include "common.h"
#include "structures/bimap.h"
#include <string>

#define KEYPAIR(x, y) std::make_pair(x, y)

class Token {
public:
  virtual std::string toString() const { return "<unknown token>"; }
};

class Word : public Token {
public:
  std::string toString() const { return "<word>"; }
};

class Keyword : public Word {
public:
  enum Kind {
    FUNCTION,
    RETURN,
    PRINTLN,
    PRINT,
    IF,
    THEN,
    ELSE,
    ENDIF,
    WHILE,
    ENDWHILE,
    COMMA,
    ASSIGN,
    OR,
    AND,
    BITWISE_OR,
    BITWISE_AND,
    BITWISE_XOR,
    EQUAL,
    NOT_EQUAL,
    LESS,
    LEQ,
    GREATER,
    GEQ,
    SHL,
    SHR,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULE,
    LEFT_PARENTHESIS,
    RIGHT_PARENTHESIS,
    LEFT_SQUARE,
    RIGHT_SQUARE,
    COLON,
    QUOTE,
    GOTO,
    GLOBAL,
    KEYWORD_N
  };
  Keyword(Kind Kind) : mKind(Kind) {}
  Keyword(const std::string &Word) {
    auto Key = mKindToName.getValue(Word);
    assert(Key && "Keyword must have a kind!");
    mKind = *Key;
  }

  static bool isKeyword(const std::string &Word) {
    return mKindToName.hasValue(Word);
  }

  std::string toString() const {
    auto Key = mKindToName.getValue(mKind);
    return Key ? ("<kw: " + *Key + ">") : "<unknown keyword>";
  }

  static bool isPunctChar(char Char) {
    return mPunctuations.find(Char) != mPunctuations.npos;
  }
  
  static const std::string &getPunctStr() { return mPunctuations; }
private:
  static const std::string mPunctuations;
  static const Bimap<Kind, std::string> mKindToName;
  Kind mKind;
};

class Identifier : public Word {
public:
  Identifier(const std::string &Name) : mName(Name) {}
  std::string toString() const { return "<id: " + mName + ">"; }
private:
  std::string mName;
};

class String : public Token {
public:
  String(const std::string &Str) : mString(Str) {}
  std::string toString() const {
    return "<literal: " + (mString.empty() ? "(empty)" : mString) + ">";
  }
private:
  std::string mString;
};

template<typename T>
class Constant : public Token {
protected:
  T mValue;
public:
  Constant(T Value) : mValue(Value) {}
  T getValue() { return mValue; }
  std::string toString() const {
    return "<number: " + std::to_string(mValue) + ">";
  }
};

class FloatConstant : public Constant<double> {
public:
  FloatConstant(double Value) : Constant<double>(Value) {}
};

class IntConstant : public Constant<int> {
public:
  IntConstant(int Value) : Constant<int>(Value) {}
};

#endif