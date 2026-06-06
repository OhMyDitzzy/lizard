#pragma once
#include <string>
#include <vector>
#include "token.hpp"

class Lexer {
public:
    Lexer(const std::string& filepath, const std::string& source);

    /* Consume the entire source and return the flat token stream. */
    std::vector<Token> tokenize();

private:
    std::string filepath_;
    std::string source_;
    size_t      pos_;
    int         line_;
    int         col_;

    char current() const;
    char peek(int offset = 1) const;
    void advance();

    void  skipWhitespace();
    void  skipLineComment();
    void  skipBlockComment();
    Token readString(char quote);
    Token readFStringToken();
    Token readMultilineString();
    Token readNumber();
    Token readIdentifierOrKeyword();
};