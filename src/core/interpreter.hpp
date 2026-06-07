#pragma once
#include "ast.hpp"
#include "environment.hpp"
#include "value.hpp"
#include <string>

class Interpreter {
  public:
    Interpreter(const std::string& filepath, const std::string& source);

    void execute(const Program& program);

  private:
    std::string filepath_;
    std::string source_;
    Environment env_;

    void executeStatement(const ASTNode& node);
    void executeShow(const ShowStatement& node);
    void executeVarDecl(const VarDeclStatement& node);
    void executeAssign(const AssignStatement& node);
    void executeIf(const IfStatement& node);
    void executeLValueAssign(const LValueAssignStatement& node);
    void executeCompoundAssign(const CompoundAssignStatement& node);

    LizardValue evalValue(const ASTNode& node);
    LizardValue evalBinary(const BinaryExprNode& node);
    LizardValue evalUnary(const UnaryExprNode& node);
    LizardValue evalComparison(const LizardValue& left, const LizardValue& right,
                               const std::string& op, const BinaryExprNode& node);
    LizardValue evalPropertyAccess(const PropertyAccessNode& node);
    LizardValue evalIndexAccess(const IndexExprNode& node);

    /* Read the current value of any lvalue expression. */
    LizardValue readLValue(const ASTNode& target);
    /* Write a value to any lvalue expression. */
    void writeLValue(const ASTNode& target, LizardValue val);

    static bool isTruthy(const LizardValue& v);
};