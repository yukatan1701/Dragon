#ifndef __DRAGON_TOKEN__
#define __DRAGON_TOKEN__

#include "dragon/Common.h"
#include "dragon/structures/Bimap.h"
#include <memory>
#include <string>
#include <vector>

#define KEYPAIR(x, y) std::make_pair(x, y)

typedef std::pair<std::size_t, std::size_t> PosInfo;

class Token {
public:
  Token() {}
  Token(const PosInfo &LineCol)
      : mLine(LineCol.first), mColumn(LineCol.second) {}
  Token(std::size_t Line, std::size_t Column) : mLine(Line), mColumn(Column) {}
  virtual std::string toString() const { return "<unknown token>"; }
  std::string getPos() const { return std::to_string(mLine) + ":" +
                                      std::to_string(mColumn); }
  virtual Token *clone() const { return new Token(); }
  virtual ~Token() {}
protected:
  std::size_t mLine, mColumn;
};

class Word : public Token {
public:
  Word() {}
  Word(const PosInfo &PI) : Token(PI) {}
  std::string toString() const { return "<word>"; }
  Token *clone() const { return new Word(); }
  ~Word() {}
};

class Keyword : public Word {
public:
  typedef int Priority;
  enum Kind {
    FUNCTION = 0,
    COLON,
    QUOTE,
    GLOBAL,

    /* brackets */
    BRACKETS_BEGIN,
    LEFT_PARENTHESIS,
    RIGHT_PARENTHESIS,
    LEFT_SQUARE,
    RIGHT_SQUARE,
    BRACKETS_END,

    /* unary operators (prefix) */
    UNARY_BEGIN,
    PRINTLN,
    PRINT,
    UNARY_PLUS,
    UNARY_MINUS,
    LOGICAL_NOT,
    RETURN,
    COMMA,
    IF,
    ELSE,
    ENDIF,
    WHILE,
    ENDWHILE,
    GOTO_UN,
    UNARY_END,

    /* binary operators */
    BINARY_BEGIN,
    ASSIGN,
    LOGICAL_OR,
    LOGICAL_AND,
    BITWISE_OR,
    BITWISE_AND,
    BITWISE_XOR,
    SHL,
    SHR,
    MODULE,
    EQUAL,
    NOT_EQUAL,

    LESS,
    LEQ,
    GREATER,
    GEQ,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    GOTO_BIN,
    BINARY_END
  };
  explicit Keyword(Kind Kind) : mKind(Kind) {}
  explicit Keyword(Kind Kind, const PosInfo &PI) : Word(PI), mKind(Kind) {}
  explicit Keyword(const std::string &WordStr, const PosInfo &PI) : Word(PI) {
    auto Key = mKindToName.getValue(WordStr);
    assert(Key && "Keyword must have a kind!");
    mKind = *Key;
  }

  static bool isKeyword(const std::string &Word) {
    return mKindToName.hasValue(Word);
  }

  std::string kindToString() const {
    auto Res = mKindToName.getValue(mKind);
    assert(Res && "Keyword must be of declared kind!");
    return *Res;
  }

  std::string toString() const {
    auto Key = mKindToName.getValue(mKind);
    return Key ? ("<kw: " + *Key + ">") : "<unknown keyword>";
  }

  Kind getKind() const { return mKind; }

  Priority getPriority() const {
    auto Itr = mKindToPriority.find(mKind);
    assert(Itr != mKindToPriority.end() && "Every keyword must have a priority!");
    return Itr->second;
  }

  Token *clone() const { return new Keyword(mKind); }

  static bool isPunctChar(char Char) {
    return mPunctuations.find(Char) != mPunctuations.npos;
  }
  
  static const std::string &getPunctStr() { return mPunctuations; }

  static void addDynamicKeyword(const std::string &Word,
                                std::vector<std::unique_ptr<Token>> &TL,
                                const PosInfo &PI);
  virtual ~Keyword() {}
private:
  static const std::string mPunctuations;
  static const Bimap<Kind, std::string> mKindToName;
  static const std::map<Kind, Priority> mKindToPriority;
  Kind mKind;
};

class PostfixOperator : public Keyword {
public:
  explicit PostfixOperator(Kind Kind) : Keyword(Kind) {}
  explicit PostfixOperator(Kind Kind, const PosInfo &PI) : Keyword(Kind, PI) {}
  Token *clone() const { return new PostfixOperator(this->getKind()); }
  virtual ~PostfixOperator() {}
};

class PrefixOperator : public Keyword {
public:
  explicit PrefixOperator(Kind Kind) : Keyword(Kind) {}
  explicit PrefixOperator(Kind Kind, const PosInfo &PI) : Keyword(Kind, PI) {}
  Token *clone() const { return new PrefixOperator(this->getKind()); }
  virtual ~PrefixOperator() {}
};

class BinaryOperator : public Keyword {
public:
  enum AssocKind { RIGHT, LEFT };
  explicit BinaryOperator(Kind Kind, AssocKind AssocKind=LEFT)
      : Keyword(Kind), mAssocKind(AssocKind) {}
  explicit BinaryOperator(Kind Kind, const PosInfo &PI,
                          AssocKind AssocKind=LEFT)
      : Keyword(Kind, PI), mAssocKind(AssocKind) {}
  AssocKind getAssocKind() const { return mAssocKind; }
  Token *clone() const { return new BinaryOperator(this->getKind(), mAssocKind); }
  virtual ~BinaryOperator() {}
private:
  AssocKind mAssocKind;
};

class Bracket : public Keyword {
public:
  explicit Bracket(Kind Kind) : Keyword(Kind) {}
  explicit Bracket(Kind Kind, const PosInfo &PI) : Keyword(Kind, PI) {}
  Token *clone() const { return new Bracket(this->getKind()); }
  virtual ~Bracket() {}
};

class Identifier : public Word {
public:
  Identifier(const std::string &Name) : mName(Name) {}
  Identifier(const std::string &Name, const PosInfo &PI)
      : Word(PI), mName(Name) {}
  std::string getName() const { return mName; }
  std::string toString() const { return "<id: " + mName + ">"; }
  Token *clone() const { return new Identifier(this->getName()); }
  Identifier *cloneIdentifier() const { return new Identifier(this->getName()); }
  virtual ~Identifier() {}
private:
  std::string mName;
};

class Constant : public Token {
public:
  Constant() {}
  Constant(const PosInfo &PI) : Token(PI) {}
  std::string toString() const { return "<unknown constant>"; }
  Token *clone() const { return new Constant(); }
  virtual Constant *cloneConst() const { return new Constant(); }
  virtual ~Constant() {}
};

class String : public Constant {
public:
  String(const std::string &Str) : mString(Str) {}
  String(const std::string &Str, const PosInfo &PI) : Constant(PI), mString(Str) {}
  std::string getValue() const { return mString; }
  std::string toString() const {
    return "<literal: " + (mString.empty() ? "(empty)" : mString) + ">";
  }
  Token *clone() const { return new String(mString); }
  virtual Constant *cloneConst() const { return new String(mString); }
  virtual ~String() {}
private:
  std::string mString;
};

class FloatConstant : public Constant {
public:
  FloatConstant(double Value) : mValue(Value) {}
  FloatConstant(double Value, const PosInfo &PI) : Constant(PI), mValue(Value) {}
  double getValue() { return mValue; }
  double setValue(double Value) { mValue = Value; return mValue; }
  std::string toString() const {
    return "<float: " + std::to_string(mValue) + ">";
  }
  Token *clone() const { return new FloatConstant(mValue); }
  virtual Constant *cloneConst() const { return new FloatConstant(mValue); }
  virtual ~FloatConstant() {}
private:
  double mValue;
};

class IntConstant : public Constant {
public:
  IntConstant(int Value) : mValue(Value) {}
  IntConstant(int Value, const PosInfo &PI) : Constant(PI), mValue(Value) {}
  int getValue() { return mValue; }
  int setValue(int Value) { mValue = Value; return mValue; }
  std::string toString() const {
    return "<int: " + std::to_string(mValue) + ">";
  }
  Token *clone() const { return new IntConstant(mValue); }
  virtual Constant *cloneConst() const { return new IntConstant(mValue); }
  virtual ~IntConstant() {}
private:
  int mValue;
};

class Boolean : public Constant {
public:
  Boolean(const std::string &Str) : mValue(Str == "true" ? true : false) {}
  Boolean(const std::string &Str, const PosInfo &PI)
      : Constant(PI), mValue(Str == "true" ? true : false) {}
  Boolean(bool Value) : mValue(Value) {}
  Boolean(bool Value, const PosInfo &PI) : Constant(PI), mValue(Value) {}
  bool getValue() { return mValue; }
  bool setValue(bool Value) { mValue = Value; return mValue; }
  std::string toString() const {
    return "<bool: " + std::string(mValue == true ? "true" : "false") + ">";
  }

  static bool isBoolean(const std::string &Str) {
    return Str == "true" || Str == "false";
  }

  Token *clone() const { return new Boolean(mValue); }
  virtual Constant *cloneConst() const { return new Boolean(mValue); }
  virtual ~Boolean() {}
private:
  bool mValue;
};

#endif