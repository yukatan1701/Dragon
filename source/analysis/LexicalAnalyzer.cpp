#include "dragon/analysis/LexicalAnalyzer.h"

LexicalAnalyzer::LexicalAnalyzer(std::istream &IS) : mLineN(0) {
  std::string Line;
  int LineN = 0;
  while (std::getline(IS, Line)) {
    parseLine(Line);
    ++LineN;
  }
  DRAGON_DEBUG(dbgs() << "[LEXICAL ANALYZER] Total line count: " <<
               LineN << "\n");
  DRAGON_DEBUG(dump());
}

void LexicalAnalyzer::parseLine(const std::string &Line) {
  CharBuffer Buffer(Line);
  ++mLineN;
  auto &TokenList = mTokens.emplace_back();
  auto isNumberChar = [](const char &Ch) {
    return std::isdigit(Ch) || Ch == '.';
  };
  auto getPos = [this, &Buffer](const CharBuffer::ConstIterator &Itr) {
    auto ColN = Itr.getPtr() - Buffer.cbegin().getPtr() + 1;
    return std::make_pair(mLineN, ColN);
  };
  auto getErrorPos = [this, &Buffer, &getPos](const CharBuffer::ConstIterator &Itr) {
    auto Pos = getPos(Itr);
    return std::to_string(Pos.first) + ":" + std::to_string(Pos.second);
  };
  const std::string CharsAfterNumber = Keyword::getPunctStr() + "# \t\n";
  for (auto Itr = Buffer.cbegin(); Itr != Buffer.cend(); ++Itr) {
    auto WordBegin = Itr;
    auto &Peek = *Itr;
    //DRAGON_DEBUG(dbgs() << "[PARSER] Current char: " << Peek << "\n");
    if (std::isspace(Peek))
      continue;
    if (std::isdigit(Peek)) {
      std::string NumStr { Peek };
      auto Next = Buffer.lookAhead(Itr);
      bool HasDot = false;
      while (Next && isNumberChar(*Next)) {
        if (HasDot && *Next == '.')
          break;
        NumStr += *Next;
        ++Itr;
        Next = Buffer.lookAhead(Itr);
      }
      if (Next && CharsAfterNumber.find(*Next) == std::string::npos)
        throw ParserException("Invalid character after number at " +
                              getErrorPos(Itr));
      if (NumStr[0] == '.' && NumStr.size() == 1)
        throw ParserException("Invalid number format at " + getErrorPos(Itr));
      if (NumStr.find('.') == std::string::npos)
        TokenList.push_back(std::make_unique<IntConstant>(std::stoi(NumStr),
                            getPos(WordBegin)));
      else
        TokenList.push_back(std::make_unique<FloatConstant>(std::stod(NumStr),
                            getPos(WordBegin)));
    } else if (std::isalpha(Peek) || Peek == '_') {
      std::string Word { Peek };
      auto Next = Buffer.lookAhead(Itr);
      while (Next && (std::isalnum(*Next) || *Next == '_')) {
        Word += *Next;
        ++Itr;
        Next = Buffer.lookAhead(Itr);
      }
      if (Keyword::isKeyword(Word)) {
        Keyword::addDynamicKeyword(Word, TokenList, getPos(WordBegin));
      } else {
        TokenList.push_back(std::make_unique<Identifier>(Word,
                            getPos(WordBegin)));
      }
    } else if (Peek == '#') {
      return;
    } else if (Keyword::isPunctChar(Peek)) {
      std::string Word { Peek };
      auto SubItr = Itr;
      auto Next = Buffer.lookAhead(SubItr);
      while (Next && Keyword::isPunctChar(*Next)) {
        Word += *Next;
        ++SubItr;
        Next = Buffer.lookAhead(SubItr);
      }
      std::size_t Len = Word.length();
      //DRAGON_DEBUG(dbgs() << "PunctWord: " << Word << "\n");
      while (Len > 0 && !Keyword::isKeyword(Word.substr(0, Len)))
        --Len;
      assert(Len > 0 && "Punctuation character must have a keyword!");
      Keyword::addDynamicKeyword(Word.substr(0, Len), TokenList,
                                 getPos(WordBegin));
      --Len;
      while (Len) {
        ++Itr;
        --Len;
      }
    } else if (Peek == '\"') {
      std::string Word = "";
      ++Itr;
      while (Itr != Buffer.cend() && *Itr != '\"') {
        Word += *Itr;
        ++Itr;
      }
      if (Itr == Buffer.cend())
        throw ParserException("Incomplete literal at " + getErrorPos(Itr));
      TokenList.push_back(std::make_unique<String>(Word, getPos(WordBegin)));
    } else {
      throw ParserException("Invalid characher at " + getErrorPos(Itr));
    }
  }
}

void LexicalAnalyzer::dump() const {
  dbgs() << "[LEXICAL ANALYZER] Printing tokens:\n";
  std::size_t LineN = 1;
  auto MaxNumLength = std::to_string(mTokens.size()).size();
  for (auto &TokenList : mTokens) {
    auto NumLength = std::to_string(LineN).size();
    std::string Space(MaxNumLength - NumLength, ' ');
    dbgs() << Space << LineN << "| ";
    for (auto &Token : TokenList) {
      dbgs() << Token.get()->toString() << " ";
    }
    dbgs() << "\n";
    ++LineN;
  }
  dbgs() << "[LEXICAL ANALYZER] End printing tokens.\n";
}