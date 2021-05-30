#include "dragon/analysis/Token.h"

const std::string Keyword::mPunctuations = "+,-*/%^|&!()[]:<>=\"";

const Bimap<Keyword::Kind, std::string> Keyword::mKindToName = {
  KEYPAIR(Keyword::FUNCTION, "function"),
  KEYPAIR(Keyword::RETURN, "return"),
  KEYPAIR(Keyword::PRINTLN, "println"),
  KEYPAIR(Keyword::PRINT, "print"),
  KEYPAIR(Keyword::IF, "if"),
  KEYPAIR(Keyword::ELSE, "else"),
  KEYPAIR(Keyword::ENDIF, "endif"),
  KEYPAIR(Keyword::WHILE, "while"),
  KEYPAIR(Keyword::ENDWHILE, "endwhile"),
  KEYPAIR(Keyword::COMMA, ","),
  KEYPAIR(Keyword::ASSIGN, "="),
  KEYPAIR(Keyword::LOGICAL_OR, "or"),
  KEYPAIR(Keyword::LOGICAL_AND, "and"),
  KEYPAIR(Keyword::LOGICAL_NOT, "!"),
  KEYPAIR(Keyword::BITWISE_OR, "|"),
  KEYPAIR(Keyword::BITWISE_AND, "&"),
  KEYPAIR(Keyword::BITWISE_XOR, "^"),
  KEYPAIR(Keyword::EQUAL, "=="),
  KEYPAIR(Keyword::NOT_EQUAL, "!="),
  KEYPAIR(Keyword::LESS, "<"),
  KEYPAIR(Keyword::LEQ, "<="),
  KEYPAIR(Keyword::GREATER, ">"),
  KEYPAIR(Keyword::GEQ, ">="),
  KEYPAIR(Keyword::SHL, "<<"),
  KEYPAIR(Keyword::SHR, ">>"),
  KEYPAIR(Keyword::PLUS, "+"),
  KEYPAIR(Keyword::MINUS, "-"),
  KEYPAIR(Keyword::MULTIPLY, "*"),
  KEYPAIR(Keyword::DIVIDE, "/"),
  KEYPAIR(Keyword::MODULE, "%"),
  KEYPAIR(Keyword::LEFT_PARENTHESIS, "("),
  KEYPAIR(Keyword::RIGHT_PARENTHESIS, ")"),
  KEYPAIR(Keyword::LEFT_SQUARE, "["),
  KEYPAIR(Keyword::RIGHT_SQUARE, "]"),
  KEYPAIR(Keyword::COLON, ":"),
  KEYPAIR(Keyword::QUOTE, "\""),
  KEYPAIR(Keyword::GOTO_BIN, "goto"),
  KEYPAIR(Keyword::GOTO_UN, "goto*"),
  KEYPAIR(Keyword::UNARY_MINUS, "-$"),
  KEYPAIR(Keyword::UNARY_PLUS, "+$"),
  KEYPAIR(Keyword::GLOBAL, "global")
};

const std::map<Keyword::Kind, Keyword::Priority> Keyword::mKindToPriority = {
  KEYPAIR(Keyword::FUNCTION, -1),
  KEYPAIR(Keyword::RETURN, 100),
  KEYPAIR(Keyword::PRINTLN, 100),
  KEYPAIR(Keyword::PRINT, 100),
  KEYPAIR(Keyword::IF, 99),
  KEYPAIR(Keyword::ELSE, -1),
  KEYPAIR(Keyword::ENDIF, -1),
  KEYPAIR(Keyword::WHILE, 99),
  KEYPAIR(Keyword::ENDWHILE, -1),
  KEYPAIR(Keyword::COLON, -1), // TODO: is it necessary?
  KEYPAIR(Keyword::QUOTE, -1),
  KEYPAIR(Keyword::GOTO_BIN, 101),
  KEYPAIR(Keyword::GOTO_UN, 101),
  KEYPAIR(Keyword::GLOBAL, 100),

  KEYPAIR(Keyword::LEFT_PARENTHESIS, 1),
  KEYPAIR(Keyword::RIGHT_PARENTHESIS, 1),
  KEYPAIR(Keyword::LEFT_SQUARE, 1),
  KEYPAIR(Keyword::RIGHT_SQUARE, 1),

  KEYPAIR(Keyword::UNARY_PLUS, 2),
  KEYPAIR(Keyword::UNARY_MINUS, 2),
  KEYPAIR(Keyword::LOGICAL_NOT, 2),

  KEYPAIR(Keyword::MULTIPLY, 3),
  KEYPAIR(Keyword::DIVIDE, 3),
  KEYPAIR(Keyword::MODULE, 3),

  KEYPAIR(Keyword::PLUS, 4),
  KEYPAIR(Keyword::MINUS, 4),

  KEYPAIR(Keyword::SHL, 5),
  KEYPAIR(Keyword::SHR, 5),

  KEYPAIR(Keyword::LESS, 6),
  KEYPAIR(Keyword::LEQ, 6),
  KEYPAIR(Keyword::GREATER, 6),
  KEYPAIR(Keyword::GEQ, 6),

  KEYPAIR(Keyword::EQUAL, 7),
  KEYPAIR(Keyword::NOT_EQUAL, 7),

  KEYPAIR(Keyword::BITWISE_AND, 8),

  KEYPAIR(Keyword::BITWISE_XOR, 9),

  KEYPAIR(Keyword::BITWISE_OR, 10),

  KEYPAIR(Keyword::LOGICAL_AND, 11),
  KEYPAIR(Keyword::LOGICAL_OR, 12),
  
  KEYPAIR(Keyword::ASSIGN, 13),

  KEYPAIR(Keyword::COMMA, 15),
};

void Keyword::addDynamicKeyword(const std::string &Word,
                                std::vector<std::unique_ptr<Token>> &TL,
                                const PosInfo &PI) {
  auto Kw = Keyword(Word, PI);
  if (Kw.mKind >= Kind::BINARY_BEGIN && Kw.mKind <= Kind::BINARY_END) {
    TL.push_back(std::make_unique<BinaryOperator>(Kw.mKind, PI,
        Kw.mKind == Kind::ASSIGN ?
            BinaryOperator::AssocKind::RIGHT :
            BinaryOperator::AssocKind::LEFT));
  } else if (Kw.mKind >= BRACKETS_BEGIN && Kw.mKind <= BRACKETS_END) {
    TL.push_back(std::make_unique<Bracket>(Kw.mKind, PI));
  } else if (Kw.mKind >= UNARY_BEGIN && Kw.mKind <= UNARY_END) {
    TL.push_back(std::make_unique<PrefixOperator>(Kw.mKind, PI));
  } else {
    TL.push_back(std::make_unique<Keyword>(Word, PI));
  }
}