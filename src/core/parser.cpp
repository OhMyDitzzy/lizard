#include "parser.hpp"
#include "lexer.hpp"
#include "../error/error.hpp"
#include "../debug.hpp"
#include <cstdlib>

Parser::Parser(const std::string& filepath,
               const std::string& source,
               const std::vector<Token>& tokens)
    : filepath_(filepath), source_(source), tokens_(tokens), pos_(0) {}

const Token& Parser::current() const { return tokens_[pos_]; }

const Token& Parser::peek(int offset) const {
    size_t idx = pos_ + static_cast<size_t>(offset);
    if (idx >= tokens_.size()) return tokens_.back();
    return tokens_[idx];
}

Token Parser::consume() {
    Token t = tokens_[pos_];
    if (pos_ < tokens_.size() - 1) pos_++;
    return t;
}

Token Parser::expect(TokenType type, const std::string& context) {
    if (current().type != type) {
        LizardError err;
        err.code     = "E010";
        err.message  = "expected " + context + ", got '" + current().value + "'";
        err.filepath = filepath_;
        err.line     = current().line;
        err.col      = current().col;
        err.length   = static_cast<int>(current().value.size());
        reportError(err, source_);
        std::exit(1);
    }
    return consume();
}

Program Parser::parse() {
    Program program;
    while (current().type != TokenType::EndOfFile) {
        program.statements.push_back(parseStatement());
        // Semicolon between statements is optional — consume any trailing ones.
        while (current().type == TokenType::Semicolon) consume();
    }
    LZ_DEBUG("parser built " << program.statements.size() << " statement(s)");
    return program;
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
    if (current().type == TokenType::Show) return parseShow();
    if (current().type == TokenType::Let)  return parseVarDecl(false);
    if (current().type == TokenType::Var)  return parseVarDecl(true);
    if (current().type == TokenType::If)   return parseIf();

    if (current().type == TokenType::Identifier) {
        if (peek().type == TokenType::Equals) return parseAssign();
        // ident followed by . or [ could be a property/index assignment
        if (peek().type == TokenType::Dot || peek().type == TokenType::LeftBracket)
            return parseLValueAssign();
    }

    LizardError err;
    err.code     = "E011";
    err.message  = "unexpected token '" + current().value + "'";
    err.filepath = filepath_;
    err.line     = current().line;
    err.col      = current().col;
    err.length   = static_cast<int>(current().value.size());
    reportError(err, source_);
    std::exit(1);
}

std::unique_ptr<VarDeclStatement> Parser::parseVarDecl(bool is_mutable) {
    auto node        = std::make_unique<VarDeclStatement>();
    node->line       = current().line;
    node->col        = current().col;
    node->is_mutable = is_mutable;
    consume();

    if (current().type != TokenType::Identifier) {
        std::string kw = is_mutable ? "var" : "let";
        LizardError err;
        err.code     = "E014";
        err.message  = "expected a variable name after '" + kw + "', got '" + current().value + "'";
        err.filepath = filepath_;
        err.line     = current().line;
        err.col      = current().col;
        err.length   = static_cast<int>(current().value.size());
        reportError(err, source_);
        std::exit(1);
    }

    node->name = consume().value;

    if (current().type == TokenType::Identifier)
        node->type_hint = consume().value;

    if (current().type == TokenType::Equals) {
        consume();
        node->initializer = parseExpr();
    } else if (!is_mutable) {
        LizardError err;
        err.code     = "E015";
        err.message  = "'let' requires an initial value";
        err.filepath = filepath_;
        err.line     = node->line;
        err.col      = node->col;
        err.length   = 3;
        err.tip      = "add '= <value>', or use 'var' if you want a zero-initialized variable";
        reportError(err, source_);
        std::exit(1);
    }

    return node;
}

std::unique_ptr<AssignStatement> Parser::parseAssign() {
    auto node   = std::make_unique<AssignStatement>();
    node->line  = current().line;
    node->col   = current().col;
    node->name  = consume().value;
    consume(); // '='
    node->value = parseExpr();
    return node;
}

std::unique_ptr<ShowStatement> Parser::parseShow() {
    auto node  = std::make_unique<ShowStatement>();
    node->line = current().line;
    node->col  = current().col;
    consume();
    expect(TokenType::LeftParen, "'(' after 'show'");

    while (current().type != TokenType::RightParen &&
           current().type != TokenType::EndOfFile)
    {
        if (current().type == TokenType::Identifier && peek().type == TokenType::Colon) {
            std::string name    = consume().value;
            int         nLine   = tokens_[pos_ - 1].line;
            int         nCol    = tokens_[pos_ - 1].col;
            consume(); // ':'

            if (name != "end_ln" && name != "start_ln" && name != "sep") {
                LizardError err;
                err.code     = "E012";
                err.message  = "unknown named argument '" + name + "' in show()";
                err.filepath = filepath_;
                err.line     = nLine; err.col = nCol;
                err.length   = static_cast<int>(name.size());
                err.tip      = "valid named arguments for show() are: end_ln, start_ln, sep";
                reportError(err, source_);
                std::exit(1);
            }
            node->named_args[name] = parseExpr();
        } else {
            node->args.push_back(parseExpr());
        }

        if (current().type == TokenType::Comma) consume();
    }

    expect(TokenType::RightParen, "')' to close show()");
    return node;
}

/* Helpers to build a BinaryExprNode cleanly. */
static std::unique_ptr<BinaryExprNode> makeBinary(std::unique_ptr<ASTNode> left,
                                                    const std::string& op,
                                                    int line, int col,
                                                    std::unique_ptr<ASTNode> right)
{
    auto node   = std::make_unique<BinaryExprNode>();
    node->line  = line;
    node->col   = col;
    node->op    = op;
    node->left  = std::move(left);
    node->right = std::move(right);
    return node;
}

std::unique_ptr<ASTNode> Parser::parseExpr() { return parseTernary(); }

std::unique_ptr<ASTNode> Parser::parseTernary() {
    auto cond = parseNullCoalesce();

    if (current().type != TokenType::Question) return cond;

    int line = current().line, col = current().col;
    consume(); // consume '?'

    auto then_expr = parseNullCoalesce();

    expect(TokenType::Colon, "':' in ternary expression");

    auto else_expr = parseTernary(); // right-associative

    auto node       = std::make_unique<TernaryExprNode>();
    node->line      = line;
    node->col       = col;
    node->condition = std::move(cond);
    node->then_expr = std::move(then_expr);
    node->else_expr = std::move(else_expr);
    return node;
}

std::unique_ptr<ASTNode> Parser::parseNullCoalesce() {
    auto left = parseLogicalOr();

    while (current().type == TokenType::QuestionQuestion) {
        int line = current().line, col = current().col;
        consume();
        left = makeBinary(std::move(left), "??", line, col, parseLogicalOr());
    }

    return left;
}
std::unique_ptr<ASTNode> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();

    while (current().type == TokenType::PipePipe || current().type == TokenType::Or) {
        int line = current().line, col = current().col;
        consume();
        left = makeBinary(std::move(left), "||", line, col, parseLogicalAnd());
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseLogicalAnd() {
    auto left = parseComparison();

    while (current().type == TokenType::AmpAmp || current().type == TokenType::And) {
        int line = current().line, col = current().col;
        consume();
        left = makeBinary(std::move(left), "&&", line, col, parseComparison());
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseComparison() {
    auto left = parseBitOr();

    if (current().type == TokenType::EqualEqual  ||
        current().type == TokenType::BangEqual    ||
        current().type == TokenType::Less         ||
        current().type == TokenType::Greater      ||
        current().type == TokenType::LessEqual    ||
        current().type == TokenType::GreaterEqual)
    {
        int         line = current().line, col = current().col;
        std::string op   = current().value;
        consume();
        left = makeBinary(std::move(left), op, line, col, parseBitOr());
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parseBitOr() {
    auto left = parseBitXor();
    while (current().type == TokenType::Pipe) {
        int line = current().line, col = current().col;
        consume();
        left = makeBinary(std::move(left), "|", line, col, parseBitXor());
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseBitXor() {
    auto left = parseBitAnd();
    while (current().type == TokenType::Caret) {
        int line = current().line, col = current().col;
        consume();
        left = makeBinary(std::move(left), "^", line, col, parseBitAnd());
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseBitAnd() {
    auto left = parseShift();
    while (current().type == TokenType::Ampersand) {
        int line = current().line, col = current().col;
        consume();
        left = makeBinary(std::move(left), "&", line, col, parseShift());
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseShift() {
    auto left = parseAdditive();
    while (current().type == TokenType::LessLess || current().type == TokenType::GreaterGreater) {
        int         line = current().line, col = current().col;
        std::string op   = current().value;
        consume();
        left = makeBinary(std::move(left), op, line, col, parseAdditive());
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseAdditive() {
    auto left = parseMult();
    while (current().type == TokenType::Plus || current().type == TokenType::Minus) {
        int         line = current().line, col = current().col;
        std::string op   = current().value;
        consume();
        left = makeBinary(std::move(left), op, line, col, parseMult());
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseMult() {
    auto left = parseUnary();
    while (current().type == TokenType::Star  ||
           current().type == TokenType::Slash ||
           current().type == TokenType::Percent)
    {
        int         line = current().line, col = current().col;
        std::string op   = current().value;
        consume();
        left = makeBinary(std::move(left), op, line, col, parseUnary());
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseUnary() {
    if (current().type == TokenType::Minus ||
        current().type == TokenType::Tilde ||
        current().type == TokenType::Bang  ||
        current().type == TokenType::Not)
    {
        int         line = current().line, col = current().col;
        std::string op   = (current().type == TokenType::Not) ? "not" : current().value;
        consume();
        auto node     = std::make_unique<UnaryExprNode>();
        node->line    = line;
        node->col     = col;
        node->op      = op;
        node->operand = parseUnary();
        return node;
    }
    return parsePostfix();
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    if (current().type == TokenType::LeftParen) {
        consume();
        auto expr = parseExpr();
        expect(TokenType::RightParen, "')' to close grouped expression");
        return expr;
    }

    auto makeLit = [&](LiteralNode::Kind kind, const std::string& val) {
        auto node   = std::make_unique<LiteralNode>();
        node->line  = current().line;
        node->col   = current().col;
        node->kind  = kind;
        node->value = val;
        consume();
        return node;
    };

    if (current().type == TokenType::String)  return makeLit(LiteralNode::Kind::String,  current().value);
    if (current().type == TokenType::Integer) return makeLit(LiteralNode::Kind::Integer, current().value);
    if (current().type == TokenType::Float)   return makeLit(LiteralNode::Kind::Float,   current().value);
    if (current().type == TokenType::True)    return makeLit(LiteralNode::Kind::Bool,    "true");
    if (current().type == TokenType::False)   return makeLit(LiteralNode::Kind::Bool,    "false");
    if (current().type == TokenType::Null)    return makeLit(LiteralNode::Kind::Null,    "null");

    if (current().type == TokenType::FString) {
        Token tok = consume();
        return parseFStringLiteral(tok);
    }

    if (current().type == TokenType::LeftBracket) return parseArrayLiteral();

    if (current().type == TokenType::Hash &&
        peek().type    == TokenType::LeftBrace)   return parseObjectLiteral();

    if (current().type == TokenType::Identifier) {
        auto node  = std::make_unique<IdentifierNode>();
        node->line = current().line;
        node->col  = current().col;
        node->name = consume().value;
        return node;
    }

    LizardError err;
    err.code     = "E013";
    err.message  = "expected a value, got '" + current().value + "'";
    err.filepath = filepath_;
    err.line     = current().line;
    err.col      = current().col;
    err.length   = static_cast<int>(current().value.size());

    if (current().type == TokenType::Let || current().type == TokenType::Var)
        err.tip = "variable declarations cannot be nested inside expressions";

    reportError(err, source_);
    std::exit(1);
}

std::vector<std::unique_ptr<ASTNode>> Parser::parseBlock() {
    std::vector<std::unique_ptr<ASTNode>> stmts;

    while (current().type != TokenType::Semicolon &&
           current().type != TokenType::Elif      &&
           current().type != TokenType::Else      &&
           current().type != TokenType::EndOfFile)
    {
        stmts.push_back(parseStatement());
    }

    if (current().type == TokenType::Semicolon) consume();

    return stmts;
}

std::vector<std::unique_ptr<ASTNode>> Parser::parseBraceBlock() {
    std::vector<std::unique_ptr<ASTNode>> stmts;
    expect(TokenType::LeftBrace, "'{'");

    while (current().type != TokenType::RightBrace &&
           current().type != TokenType::EndOfFile)
    {
        stmts.push_back(parseStatement());
        // Inside braces, trailing ';' after a statement is optional but allowed.
        if (current().type == TokenType::Semicolon) consume();
    }

    expect(TokenType::RightBrace, "'}' to close block");
    return stmts;
}

/*
 * Decide which block style to use based on what follows the keyword:
 *   '{'              -> brace block
 *   same line        -> single statement, no terminator
 *   next line        -> indentation block terminated by ';'
 */
std::vector<std::unique_ptr<ASTNode>> Parser::parseIfBody(int keyword_line) {
    if (current().type == TokenType::LeftBrace) {
        return parseBraceBlock();
    }

    std::vector<std::unique_ptr<ASTNode>> body;

    if (current().line == keyword_line) {
        body.push_back(parseStatement());
    } else {
        body = parseBlock();
    }

    return body;
}

std::unique_ptr<IfStatement> Parser::parseIf() {
    auto node  = std::make_unique<IfStatement>();
    node->line = current().line;
    node->col  = current().col;

    int if_line = current().line;
    consume(); // consume 'if'

    node->condition = parseExpr();
    node->then_body = parseIfBody(if_line);

    while (current().type == TokenType::Elif) {
        ElifClause clause;
        int elif_line = current().line;
        consume(); // consume 'elif'

        clause.condition = parseExpr();
        clause.body      = parseIfBody(elif_line);
        node->elif_clauses.push_back(std::move(clause));
    }

    if (current().type == TokenType::Else) {
        int else_line = current().line;
        consume(); // consume 'else'
        node->else_body = parseIfBody(else_line);
    }

    return node;
}

/*
 * Convert a format string token into a left-associative concatenation tree.
 *
 * The raw token value holds everything between the quotes, e.g.:
 *   "Hello {name}, you are {age + 1} years old!"
 *
 * Text segments have their escape sequences processed here.
 * Each {expr} is sub-lexed and sub-parsed into a real expression node.
 * {{ becomes a literal { and }} becomes a literal }.
 */
std::unique_ptr<ASTNode> Parser::parseFStringLiteral(const Token& token) {
    const std::string& raw = token.value;
    std::vector<std::unique_ptr<ASTNode>> parts;
    std::string text_buf;
    size_t i = 0;

    auto flushText = [&]() {
        if (text_buf.empty()) return;
        auto lit   = std::make_unique<LiteralNode>();
        lit->line  = token.line; lit->col = token.col;
        lit->kind  = LiteralNode::Kind::String;
        lit->value = text_buf;
        parts.push_back(std::move(lit));
        text_buf.clear();
    };

    while (i < raw.size()) {
        if (i + 1 < raw.size() && raw[i] == '{' && raw[i + 1] == '{') { text_buf += '{'; i += 2; continue; }
        if (i + 1 < raw.size() && raw[i] == '}' && raw[i + 1] == '}') { text_buf += '}'; i += 2; continue; }

        if (raw[i] == '{') {
            flushText();
            i++; // skip '{'
            int depth = 1;
            std::string expr_src;
            while (i < raw.size() && depth > 0) {
                if      (raw[i] == '{') depth++;
                else if (raw[i] == '}') { depth--; if (depth == 0) break; }
                expr_src += raw[i++];
            }
            i++; // skip '}'
            Lexer  sub_lex(filepath_, expr_src);
            Parser sub_par(filepath_, expr_src, sub_lex.tokenize());
            parts.push_back(sub_par.parseExpr());
            continue;
        }

        if (raw[i] == '\\' && i + 1 < raw.size()) {
            switch (raw[++i]) {
                case 'n':  text_buf += '\n'; break;
                case 't':  text_buf += '\t'; break;
                case 'r':  text_buf += '\r'; break;
                case '"':  text_buf += '"';  break;
                case '\'': text_buf += '\''; break;
                case '\\': text_buf += '\\'; break;
                case 'e':  text_buf += '\033'; break;
                default:   text_buf += raw[i]; break;
            }
            i++; continue;
        }

        text_buf += raw[i++];
    }

    flushText();

    if (parts.empty()) {
        auto empty  = std::make_unique<LiteralNode>();
        empty->line = token.line; empty->col = token.col;
        empty->kind = LiteralNode::Kind::String; empty->value = "";
        return empty;
    }

    auto result = std::move(parts[0]);
    for (size_t j = 1; j < parts.size(); j++) {
        auto bin   = std::make_unique<BinaryExprNode>();
        bin->line  = token.line; bin->col = token.col; bin->op = "+";
        bin->left  = std::move(result);
        bin->right = std::move(parts[j]);
        result     = std::move(bin);
    }

    return result;
}

std::unique_ptr<ASTNode> Parser::parsePostfix() {
    auto node = parsePrimary();

    while (current().type == TokenType::Dot ||
           current().type == TokenType::LeftBracket)
    {
        if (current().type == TokenType::Dot) {
            int line = current().line, col = current().col;
            consume(); // consume '.'
            if (current().type != TokenType::Identifier) {
                LizardError err;
                err.code = "E017"; err.message = "expected property name after '.'";
                err.filepath = filepath_; err.line = current().line; err.col = current().col;
                err.length = 1;
                reportError(err, source_); std::exit(1);
            }
            auto acc      = std::make_unique<PropertyAccessNode>();
            acc->line     = line; acc->col = col;
            acc->object   = std::move(node);
            acc->property = consume().value;
            node = std::move(acc);
        } else {
            int line = current().line, col = current().col;
            consume(); // consume '['
            auto idx   = std::make_unique<IndexExprNode>();
            idx->line  = line; idx->col = col;
            idx->object = std::move(node);
            idx->index  = parseExpr();
            expect(TokenType::RightBracket, "']' to close index expression");
            node = std::move(idx);
        }
    }

    return node;
}

std::unique_ptr<ASTNode> Parser::parseArrayLiteral() {
    auto node  = std::make_unique<ArrayLiteralNode>();
    node->line = current().line; node->col = current().col;
    consume(); // consume '['

    while (current().type != TokenType::RightBracket &&
           current().type != TokenType::EndOfFile)
    {
        node->elements.push_back(parseExpr());
        if (current().type == TokenType::Comma) consume();
    }

    expect(TokenType::RightBracket, "']' to close array literal");
    return node;
}

std::unique_ptr<ASTNode> Parser::parseObjectLiteral() {
    auto node  = std::make_unique<ObjectLiteralNode>();
    node->line = current().line; node->col = current().col;
    consume(); // consume '#'
    consume(); // consume '{'

    while (current().type != TokenType::RightBrace &&
           current().type != TokenType::EndOfFile)
    {
        ObjectPair pair;

        // Key: bare identifier or quoted string
        if (current().type == TokenType::Identifier) {
            pair.key = consume().value;
        } else if (current().type == TokenType::String) {
            pair.key = consume().value;
        } else {
            LizardError err;
            err.code = "E018"; err.message = "expected a key (identifier or string) in object literal";
            err.filepath = filepath_; err.line = current().line; err.col = current().col;
            err.length = static_cast<int>(current().value.size());
            reportError(err, source_); std::exit(1);
        }

        expect(TokenType::Colon, "':' after key in object literal");
        pair.value = parseExpr();
        node->pairs.push_back(std::move(pair));

        if (current().type == TokenType::Comma) consume();
    }

    expect(TokenType::RightBrace, "'}' to close object literal");
    return node;
}

std::unique_ptr<LValueAssignStatement> Parser::parseLValueAssign() {
    auto node  = std::make_unique<LValueAssignStatement>();
    node->line = current().line; node->col = current().col;

    // Parse the left-hand side: ident (.prop | [expr])+
    auto base = std::make_unique<IdentifierNode>();
    base->line = current().line; base->col = current().col;
    base->name = consume().value;

    std::unique_ptr<ASTNode> target = std::move(base);

    while (current().type == TokenType::Dot ||
           current().type == TokenType::LeftBracket)
    {
        if (current().type == TokenType::Dot) {
            int line = current().line, col = current().col;
            consume();
            if (current().type != TokenType::Identifier) {
                LizardError err;
                err.code = "E017"; err.message = "expected property name after '.'";
                err.filepath = filepath_; err.line = current().line; err.col = current().col;
                err.length = 1;
                reportError(err, source_); std::exit(1);
            }
            auto acc      = std::make_unique<PropertyAccessNode>();
            acc->line     = line; acc->col = col;
            acc->object   = std::move(target);
            acc->property = consume().value;
            target = std::move(acc);
        } else {
            int line = current().line, col = current().col;
            consume(); // '['
            auto idx    = std::make_unique<IndexExprNode>();
            idx->line   = line; idx->col = col;
            idx->object = std::move(target);
            idx->index  = parseExpr();
            expect(TokenType::RightBracket, "']' to close index expression");
            target = std::move(idx);
        }
    }

    expect(TokenType::Equals, "'=' in assignment");

    node->target = std::move(target);
    node->value  = parseExpr();
    return node;
}