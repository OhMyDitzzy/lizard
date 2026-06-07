#include "lexer.hpp"
#include "../debug.hpp"
#include "../error/error.hpp"
#include <cctype>
#include <cstdlib>

Lexer::Lexer(const std::string& filepath, const std::string& source)
    : filepath_(filepath), source_(source), pos_(0), line_(1), col_(1) {}

char Lexer::current() const {
    if (pos_ >= source_.size()) return '\0';
    return source_[pos_];
}

char Lexer::peek(int offset) const {
    size_t target = pos_ + static_cast<size_t>(offset);
    if (target >= source_.size()) return '\0';
    return source_[target];
}

void Lexer::advance() {
    if (pos_ >= source_.size()) return;
    if (source_[pos_] == '\n') {
        line_++;
        col_ = 1;
    } else {
        col_++;
    }
    pos_++;
}

void Lexer::skipWhitespace() {
    while (pos_ < source_.size()) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            advance();
        else
            break;
    }
}

void Lexer::skipLineComment() {
    while (pos_ < source_.size() && current() != '\n')
        advance();
}

void Lexer::skipBlockComment() {
    int startLine = line_, startCol = col_;
    while (pos_ < source_.size()) {
        if (current() == '*' && peek() == '/') {
            advance();
            advance();
            return;
        }
        advance();
    }
    LizardError err;
    err.code = "E003";
    err.message = "unterminated block comment";
    err.filepath = filepath_;
    err.line = startLine;
    err.col = startCol;
    err.length = 2;
    err.tip = "block comments must be closed with */";
    reportError(err, source_);
    std::exit(1);
}

Token Lexer::readString(char quote) {
    int startLine = line_, startCol = col_;
    advance(); // consume opening quote

    std::string value;
    while (pos_ < source_.size() && current() != quote) {
        if (current() == '\n') break;

        if (current() == '\\') {
            advance();
            char esc = current();

            if (esc == 'n') {
                value += '\n';
                advance();
            } else if (esc == 't') {
                value += '\t';
                advance();
            } else if (esc == 'r') {
                value += '\r';
                advance();
            } else if (esc == '"') {
                value += '"';
                advance();
            } else if (esc == '\'') {
                value += '\'';
                advance();
            } else if (esc == '\\') {
                value += '\\';
                advance();
            } else if (esc == 'e') {
                value += '\033';
                advance();
            } else if (esc == '0' && (peek() < '0' || peek() > '7')) {
                value += '\0';
                advance();
            } else if (esc == 'x') {
                advance();
                std::string hex;
                while (hex.size() < 2 && pos_ < source_.size() && std::isxdigit(current())) {
                    hex += current();
                    advance();
                }
                value += hex.empty() ? 'x' : static_cast<char>(std::stoi(hex, nullptr, 16));
            } else if (esc >= '0' && esc <= '7') {
                std::string oct(1, esc);
                advance();
                while (oct.size() < 3 && pos_ < source_.size() && current() >= '0' &&
                       current() <= '7') {
                    oct += current();
                    advance();
                }
                value += static_cast<char>(std::stoi(oct, nullptr, 8));
            } else {
                value += esc;
                advance();
            }
        } else {
            value += current();
            advance();
        }
    }

    if (pos_ >= source_.size() || current() != quote) {
        LizardError err;
        err.code = "E001";
        err.message = "unterminated string literal";
        err.filepath = filepath_;
        err.line = startLine;
        err.col = startCol;
        err.length = 1;
        err.tip = std::string("string literals must be closed with a matching ") + quote;
        reportError(err, source_);
        std::exit(1);
    }

    advance(); // consume closing quote
    return Token{TokenType::String, value, startLine, startCol};
}

/*
 * Read a backtick multiline string.
 * Content is taken verbatim — newlines and indentation are preserved.
 * The only escape recognized is \` so a literal backtick can appear in the string.
 */
Token Lexer::readMultilineString() {
    int startLine = line_, startCol = col_;
    advance(); // consume opening `

    std::string value;
    while (pos_ < source_.size()) {
        char c = current();

        if (c == '`') {
            advance(); // consume closing `
            return Token{TokenType::String, value, startLine, startCol};
        }

        if (c == '\\' && peek() == '`') {
            value += '`';
            advance();
            advance();
            continue;
        }

        value += c;
        advance();
    }

    LizardError err;
    err.code = "E001";
    err.message = "unterminated multiline string";
    err.filepath = filepath_;
    err.line = startLine;
    err.col = startCol;
    err.length = 1;
    err.tip = "multiline strings must be closed with a matching '`'";
    reportError(err, source_);
    std::exit(1);
}

Token Lexer::readNumber() {
    int startLine = line_, startCol = col_;

    // Binary literal: 0b1010 or 0B1010
    if (current() == '0' && (peek() == 'b' || peek() == 'B')) {
        advance();
        advance(); // consume '0' and 'b'
        std::string bits;
        while (pos_ < source_.size() && (current() == '0' || current() == '1')) {
            bits += current();
            advance();
        }
        if (bits.empty()) {
            LizardError err;
            err.code = "E006";
            err.message = "expected binary digits after '0b'";
            err.filepath = filepath_;
            err.line = startLine;
            err.col = startCol;
            err.length = 2;
            err.tip = "binary literals use only 0 and 1, e.g. 0b1010";
            reportError(err, source_);
            std::exit(1);
        }
        return Token{TokenType::Integer, std::to_string(std::stoll(bits, nullptr, 2)), startLine,
                     startCol};
    }

    // Hex literal: 0xFF or 0XFF
    if (current() == '0' && (peek() == 'x' || peek() == 'X')) {
        advance();
        advance(); // consume '0' and 'x'
        std::string digits;
        while (pos_ < source_.size() && std::isxdigit(current())) {
            digits += current();
            advance();
        }
        if (digits.empty()) {
            LizardError err;
            err.code = "E007";
            err.message = "expected hex digits after '0x'";
            err.filepath = filepath_;
            err.line = startLine;
            err.col = startCol;
            err.length = 2;
            err.tip = "hex literals use 0-9 and a-f, e.g. 0xFF";
            reportError(err, source_);
            std::exit(1);
        }
        return Token{TokenType::Integer, std::to_string(std::stoll(digits, nullptr, 16)), startLine,
                     startCol};
    }

    // Decimal and float
    std::string value;
    bool isFloat = false;

    while (pos_ < source_.size()) {
        char c = current();
        if (std::isdigit(c)) {
            value += c;
            advance();
        } else if (c == '.' && !isFloat && std::isdigit(peek())) {
            isFloat = true;
            value += c;
            advance();
        } else
            break;
    }

    return Token{isFloat ? TokenType::Float : TokenType::Integer, value, startLine, startCol};
}

Token Lexer::readIdentifierOrKeyword() {
    int startLine = line_, startCol = col_;
    std::string value;

    while (pos_ < source_.size() && (std::isalnum(current()) || current() == '_')) {
        value += current();
        advance();
    }

    if (value == "show") return Token{TokenType::Show, value, startLine, startCol};
    if (value == "let") return Token{TokenType::Let, value, startLine, startCol};
    if (value == "var") return Token{TokenType::Var, value, startLine, startCol};
    if (value == "true") return Token{TokenType::True, value, startLine, startCol};
    if (value == "false") return Token{TokenType::False, value, startLine, startCol};
    if (value == "null") return Token{TokenType::Null, value, startLine, startCol};
    if (value == "and") return Token{TokenType::And, value, startLine, startCol};
    if (value == "or") return Token{TokenType::Or, value, startLine, startCol};
    if (value == "not") return Token{TokenType::Not, value, startLine, startCol};
    if (value == "if") return Token{TokenType::If, value, startLine, startCol};
    if (value == "elif") return Token{TokenType::Elif, value, startLine, startCol};
    if (value == "else") return Token{TokenType::Else, value, startLine, startCol};

    return Token{TokenType::Identifier, value, startLine, startCol};
}

/*
 * Read a format string literal: f#"..." f#'...' or f#`...`
 * The raw content between quotes is stored in the token value as-is,
 * including any {expr} placeholders. The parser converts them into
 * a string concatenation expression tree.
 */
Token Lexer::readFStringToken() {
    int startLine = line_, startCol = col_;

    advance(); // consume 'f'
    advance(); // consume '#'

    char quote = current();
    advance(); // consume opening quote

    std::string raw;
    int depth = 0; // tracks depth of { } so a quote inside {expr} doesn't end the string

    while (pos_ < source_.size()) {
        char c = current();

        if (depth == 0 && c == quote) {
            advance(); // consume closing quote
            return Token{TokenType::FString, raw, startLine, startCol};
        }

        // Newline terminates single-line fstrings ("" and '') but not backtick ones.
        if (depth == 0 && c == '\n' && quote != '`') break;

        if (c == '{')
            depth++;
        else if (c == '}' && depth > 0)
            depth--;

        raw += c;
        advance();
    }

    LizardError err;
    err.code = "E001";
    err.message = "unterminated format string";
    err.filepath = filepath_;
    err.line = startLine;
    err.col = startCol;
    err.length = 2;
    err.tip = std::string("format strings must end with a matching ") + quote;
    reportError(err, source_);
    std::exit(1);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespace();
        if (pos_ >= source_.size()) {
            tokens.push_back(Token{TokenType::EndOfFile, "", line_, col_});
            break;
        }

        if (current() == '/' && peek() == '/') {
            advance();
            advance();
            skipLineComment();
            continue;
        }
        if (current() == '/' && peek() == '*') {
            advance();
            advance();
            skipBlockComment();
            continue;
        }

        char c = current();
        int tokenLine = line_;
        int tokenCol = col_;

        if (c == '"' || c == '\'') {
            tokens.push_back(readString(c));
        } else if (c == '`') {
            tokens.push_back(readMultilineString());
        } else if (std::isdigit(c)) {
            tokens.push_back(readNumber());
        } else if (c == 'f' && peek() == '#' &&
                   (peek(2) == '"' || peek(2) == '\'' || peek(2) == '`')) {
            tokens.push_back(readFStringToken());
        } else if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        } else if (c == '(') {
            advance();
            tokens.push_back(Token{TokenType::LeftParen, "(", tokenLine, tokenCol});
        } else if (c == ')') {
            advance();
            tokens.push_back(Token{TokenType::RightParen, ")", tokenLine, tokenCol});
        } else if (c == '[') {
            advance();
            tokens.push_back(Token{TokenType::LeftBracket, "[", tokenLine, tokenCol});
        } else if (c == ']') {
            advance();
            tokens.push_back(Token{TokenType::RightBracket, "]", tokenLine, tokenCol});
        } else if (c == '.') {
            advance();
            tokens.push_back(Token{TokenType::Dot, ".", tokenLine, tokenCol});
        } else if (c == '#') {
            advance();
            tokens.push_back(Token{TokenType::Hash, "#", tokenLine, tokenCol});
        } else if (c == ',') {
            advance();
            tokens.push_back(Token{TokenType::Comma, ",", tokenLine, tokenCol});
        } else if (c == ':') {
            advance();
            tokens.push_back(Token{TokenType::Colon, ":", tokenLine, tokenCol});
        } else if (c == ';') {
            advance();
            tokens.push_back(Token{TokenType::Semicolon, ";", tokenLine, tokenCol});
        } else if (c == '{') {
            advance();
            tokens.push_back(Token{TokenType::LeftBrace, "{", tokenLine, tokenCol});
        } else if (c == '}') {
            advance();
            tokens.push_back(Token{TokenType::RightBrace, "}", tokenLine, tokenCol});
        } else if (c == '+') {
            advance();
            if (current() == '+') {
                advance();
                tokens.push_back(Token{TokenType::PlusPlus, "++", tokenLine, tokenCol});
            } else if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::PlusEquals, "+=", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Plus, "+", tokenLine, tokenCol});
            }
        } else if (c == '-') {
            advance();
            if (current() == '-') {
                advance();
                tokens.push_back(Token{TokenType::MinusMinus, "--", tokenLine, tokenCol});
            } else if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::MinusEquals, "-=", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Minus, "-", tokenLine, tokenCol});
            }
        } else if (c == '*') {
            advance();
            if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::StarEquals, "*=", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Star, "*", tokenLine, tokenCol});
            }
        } else if (c == '/') {
            advance();
            if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::SlashEquals, "/=", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Slash, "/", tokenLine, tokenCol});
            }
        } else if (c == '%') {
            advance();
            if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::PercentEquals, "%=", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Percent, "%", tokenLine, tokenCol});
            }
        } else if (c == '^') {
            advance();
            if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::CaretEquals, "^=", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Caret, "^", tokenLine, tokenCol});
            }
        } else if (c == '~') {
            advance();
            tokens.push_back(Token{TokenType::Tilde, "~", tokenLine, tokenCol});
        } else if (c == '=') {
            advance();
            if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::EqualEqual, "==", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Equals, "=", tokenLine, tokenCol});
            }
        } else if (c == '!') {
            advance();
            if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::BangEqual, "!=", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Bang, "!", tokenLine, tokenCol});
            }
        } else if (c == '?') {
            advance();
            if (current() == '?') {
                advance();
                tokens.push_back(Token{TokenType::QuestionQuestion, "??", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Question, "?", tokenLine, tokenCol});
            }
        } else if (c == '&') {
            advance();
            if (current() == '&') {
                advance();
                tokens.push_back(Token{TokenType::AmpAmp, "&&", tokenLine, tokenCol});
            } else if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::AmpersandEquals, "&=", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Ampersand, "&", tokenLine, tokenCol});
            }
        } else if (c == '|') {
            advance();
            if (current() == '|') {
                advance();
                tokens.push_back(Token{TokenType::PipePipe, "||", tokenLine, tokenCol});
            } else if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::PipeEquals, "|=", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Pipe, "|", tokenLine, tokenCol});
            }
        } else if (c == '<') {
            advance();
            if (current() == '<') {
                advance();
                if (current() == '=') {
                    advance();
                    tokens.push_back(Token{TokenType::LessLessEquals, "<<=", tokenLine, tokenCol});
                } else {
                    tokens.push_back(Token{TokenType::LessLess, "<<", tokenLine, tokenCol});
                }
            } else if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::LessEqual, "<=", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Less, "<", tokenLine, tokenCol});
            }
        } else if (c == '>') {
            advance();
            if (current() == '>') {
                advance();
                if (current() == '=') {
                    advance();
                    tokens.push_back(
                        Token{TokenType::GreaterGreaterEquals, ">>=", tokenLine, tokenCol});
                } else {
                    tokens.push_back(Token{TokenType::GreaterGreater, ">>", tokenLine, tokenCol});
                }
            } else if (current() == '=') {
                advance();
                tokens.push_back(Token{TokenType::GreaterEqual, ">=", tokenLine, tokenCol});
            } else {
                tokens.push_back(Token{TokenType::Greater, ">", tokenLine, tokenCol});
            }
        } else {
            LizardError err;
            err.code = "E002";
            err.message = std::string("unexpected character '") + c + "'";
            err.filepath = filepath_;
            err.line = tokenLine;
            err.col = tokenCol;
            err.length = 1;
            reportError(err, source_);
            std::exit(1);
        }
    }

    LZ_DEBUG("lexer produced " << tokens.size() << " tokens");
    return tokens;
}