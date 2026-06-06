#pragma once
#include <string>

enum class TokenType {
    /* Literal values */
    String,
    Integer,
    Float,
    FString,   // f#"..." format string

    /* Keywords */
    Show,
    Let,
    Var,
    True,
    False,
    Null,
    And,
    Or,
    Not,
    If,
    Elif,
    Else,

    /* Identifiers */
    Identifier,

    /* Arithmetic operators */
    Plus,            // +
    Minus,           // -
    Star,            // *
    Slash,           // /
    Percent,         // %

    /* Bitwise operators */
    Ampersand,       // &
    Pipe,            // |
    Caret,           // ^
    Tilde,           // ~
    LessLess,        // <<
    GreaterGreater,  // >>

    /* Comparison operators */
    EqualEqual,      // ==
    BangEqual,       // !=
    Less,            // <
    Greater,         // >
    LessEqual,       // <=
    GreaterEqual,    // >=

    /* Logical operators (symbol form) */
    AmpAmp,          // &&
    PipePipe,        // ||
    Bang,            // !
    Question,        // ?
    QuestionQuestion, // ??

    /* Punctuation */
    LeftParen,       // (
    RightParen,      // )
    LeftBracket,     // [
    RightBracket,    // ]
    Comma,           // ,
    Colon,           // :
    Dot,             // .
    Hash,            // #
    Equals,          // =
    Semicolon,       // ;
    LeftBrace,       // {
    RightBrace,      // }

    EndOfFile
};

struct Token {
    TokenType   type;
    std::string value;
    int         line;
    int         col;
};