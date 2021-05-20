#ifndef __DRAGON_LEXICAL_ANALYZER__
#define __DRAGON_LEXICAL_ANALYZER__

#include "dragon/analysis/Token.h"
#include <iostream>
#include <istream>
#include <vector>
#include <memory>
#include <optional>
#include <exception>

class CharBuffer {
public:
  struct ConstIterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = char;
    using pointer = const char*;
    using reference = const char&;
    
    ConstIterator(pointer Ptr) : mPtr(Ptr) {}

    const reference operator*() const { return *mPtr; }
    
    pointer operator->() { return mPtr; }
    
    ConstIterator operator++() { mPtr++; return *this; }
    
    ConstIterator operator++(int) {
      ConstIterator Tmp = *this;
      ++(*this);
      return Tmp;
    }

    friend bool operator==(const ConstIterator &LHS, const ConstIterator &RHS) {
      return LHS.mPtr == RHS.mPtr;
    }

    friend bool operator!=(const ConstIterator &LHS, const ConstIterator &RHS) {
      return LHS.mPtr != RHS.mPtr;
    }

    pointer getPtr() const { return mPtr; }

  private:
    pointer mPtr;
  };
  CharBuffer(const std::string &String) : mString(String) {}
  
  ConstIterator begin() { return ConstIterator(&mString[0]); }
  ConstIterator end() { return ConstIterator(&mString[mString.length()]); }

  ConstIterator cbegin() const { return ConstIterator(&mString[0]); }
  ConstIterator cend() const { return ConstIterator(&mString[mString.length()]);}

  std::optional<char> lookAhead(const ConstIterator &Itr) const {
    if (Itr == cend() || std::next(Itr) == cend())
      return std::nullopt;
    return *std::next(Itr);
  };

private:
  const std::string &mString;
};

class ParserException : public std::exception {
public:
  ParserException(const std::string &Msg) {
    mMsg = "[PARSER EXCEPTION] " + Msg + ".\n";
  }
  virtual const char *what() const noexcept { return mMsg.c_str(); }
private:
  std::string mMsg;
};

class LexicalAnalyzer {
public:
  typedef std::vector<std::vector<std::unique_ptr<Token>>> TokenList;
  LexicalAnalyzer(std::istream &IS);
  const TokenList &getTokenList() const { return mTokens; }
  void dump() const;
private:
  void parseLine(const std::string &Line);
  TokenList mTokens;
  int mLineN;
};

#endif