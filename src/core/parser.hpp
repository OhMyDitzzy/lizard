#pragma once
#include <string>
#include <vector>
#include <memory>
#include "token.hpp"
#include "ast.hpp"

class Parser {
public:
    Parser(const std::string& filepath,
           const std::string& source,
           const std::vector<Token>& tokens);

    Program parse();

private:
    std::string        filepath_;
    std::string        source_;
    std::vector<Token> tokens_;
    size_t             pos_;

    const Token& current() const;
    const Token& peek(int offset = 1) const;
    Token consume();
    Token expect(TokenType type, const std::string& context);

    /* Statement parsers */
    std::unique_ptr<ASTNode>          parseStatement();
    std::unique_ptr<VarDeclStatement> parseVarDecl(bool is_mutable);
    std::unique_ptr<AssignStatement>  parseAssign();
    std::unique_ptr<ShowStatement>    parseShow();
    std::unique_ptr<IfStatement>      parseIf();

    /* Parse statements until ';' and return them as a block. */
    std::vector<std::unique_ptr<ASTNode>> parseBlock();

    /* Parse a '{' ... '}' delimited block. */
    std::vector<std::unique_ptr<ASTNode>> parseBraceBlock();

    /*
     * Parse whichever block style follows an if/elif/else keyword.
     * keyword_line is the line of the keyword so same-line detection works.
     */
    std::vector<std::unique_ptr<ASTNode>> parseIfBody(int keyword_line);

    /*
     * Expression parsers — one per precedence level, lowest to highest:
     *
     *   parseExpr       -> parseLogicalOr
     *   parseLogicalOr  -> parseLogicalAnd  ('||'|'or'         parseLogicalAnd)*
     *   parseLogicalAnd -> parseComparison  ('&&'|'and'        parseComparison)*
     *   parseComparison -> parseBitOr       ('=='|'!='|'<'|... parseBitOr)?
     *   parseBitOr      -> parseBitXor      ('|'               parseBitXor)*
     *   parseBitXor     -> parseBitAnd      ('^'               parseBitAnd)*
     *   parseBitAnd     -> parseShift       ('&'               parseShift)*
     *   parseShift      -> parseAdditive    ('<<'|'>>'         parseAdditive)*
     *   parseAdditive   -> parseMult        ('+'|'-'           parseMult)*
     *   parseMult       -> parseUnary       ('*'|'/'|'%'       parseUnary)*
     *   parseUnary      -> ('-'|'~'|'!'|'not') parseUnary | parsePrimary
     *   parsePrimary    -> literal | null | identifier | '(' parseExpr ')'
     */
    std::unique_ptr<ASTNode> parseExpr();
    std::unique_ptr<ASTNode> parseTernary();
    std::unique_ptr<ASTNode> parseNullCoalesce();
    std::unique_ptr<ASTNode> parseLogicalOr();
    std::unique_ptr<ASTNode> parseLogicalAnd();
    std::unique_ptr<ASTNode> parseComparison();
    std::unique_ptr<ASTNode> parseBitOr();
    std::unique_ptr<ASTNode> parseBitXor();
    std::unique_ptr<ASTNode> parseBitAnd();
    std::unique_ptr<ASTNode> parseShift();
    std::unique_ptr<ASTNode> parseAdditive();
    std::unique_ptr<ASTNode> parseMult();
    std::unique_ptr<ASTNode> parseUnary();
    /* parsePostfix handles . and [] chains after a primary — called by parseUnary. */
    std::unique_ptr<ASTNode> parsePostfix();
    std::unique_ptr<ASTNode> parsePrimary();
    std::unique_ptr<ASTNode> parseArrayLiteral();
    std::unique_ptr<ASTNode> parseObjectLiteral();
    /* Parse ident(.prop|[idx])+ = expr as a statement. */
    std::unique_ptr<LValueAssignStatement> parseLValueAssign();

    /* Build a concat expression tree from a format string token. */
    std::unique_ptr<ASTNode> parseFStringLiteral(const Token& token);
};