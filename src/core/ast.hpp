#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

/*
 * Every node stores its source location so the interpreter can
 * produce accurate error messages that point back to the right
 * line and column even for runtime problems.
 */
struct ASTNode {
    int line = 0;
    int col  = 0;
    virtual ~ASTNode() = default;
};

/* A literal value: string, integer, float, bool, or null. */
struct LiteralNode : ASTNode {
    enum class Kind { String, Integer, Float, Bool, Null };
    Kind        kind;
    std::string value;
};

/* A reference to a variable by name: show(x), var y = x, etc. */
struct IdentifierNode : ASTNode {
    std::string name;
};

/*
 * Variable declaration.
 *
 *   let name string = "Hello"   (immutable, explicit type)
 *   let name = "Hello"          (immutable, type inferred)
 *   var count int = 0           (mutable, explicit type)
 *   var count = 0               (mutable, type inferred)
 *
 * type_hint is empty when the user omits the type annotation.
 * It is stored but not used for type checking — that is left
 * for a future static analysis tool.
 */
struct VarDeclStatement : ASTNode {
    bool        is_mutable;
    std::string name;
    std::string type_hint;
    std::unique_ptr<ASTNode> initializer;
};

/*
 * Reassignment to an existing variable.
 *
 *   count = 99
 *
 * The interpreter enforces that the target was declared with 'var'.
 */
struct AssignStatement : ASTNode {
    std::string name;
    std::unique_ptr<ASTNode> value;
};

/*
 * A show() call.
 *
 * Positional args land in `args` in source order.
 * Named args (end_ln, start_ln, sep) are keyed by name.
 * Both can hold any evaluable expression, not just literals.
 */
struct ShowStatement : ASTNode {
    std::vector<std::unique_ptr<ASTNode>>                    args;
    std::unordered_map<std::string, std::unique_ptr<ASTNode>> named_args;
};

/* Binary expression: left op right. The op field holds the operator string. */
struct BinaryExprNode : ASTNode {
    std::string              op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
};

/* Unary expression: op operand. Currently supports '-', '~', '!', 'not'. */
struct UnaryExprNode : ASTNode {
    std::string              op;
    std::unique_ptr<ASTNode> operand;
};

/* Ternary expression: condition ? then_expr : else_expr */
struct TernaryExprNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> then_expr;
    std::unique_ptr<ASTNode> else_expr;
};

/* One elif branch: its condition and the statements to run if it matches. */
struct ElifClause {
    std::unique_ptr<ASTNode>              condition;
    std::vector<std::unique_ptr<ASTNode>> body;

    ElifClause() = default;
    ElifClause(ElifClause&&) = default;
    ElifClause& operator=(ElifClause&&) = default;
};

/*
 * if condition stmt
 * if condition
 *     stmts;
 * elif / else branches are optional.
 */
struct IfStatement : ASTNode {
    std::unique_ptr<ASTNode>              condition;
    std::vector<std::unique_ptr<ASTNode>> then_body;
    std::vector<ElifClause>               elif_clauses;
    std::vector<std::unique_ptr<ASTNode>> else_body;
};

/* Array literal: [expr, expr, ...] */
struct ArrayLiteralNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> elements;
};

/* One key-value pair inside an object literal. */
struct ObjectPair {
    std::string              key;
    std::unique_ptr<ASTNode> value;

    ObjectPair() = default;
    ObjectPair(ObjectPair&&) = default;
    ObjectPair& operator=(ObjectPair&&) = default;
};

/* Object literal: #{ key: expr, ... } */
struct ObjectLiteralNode : ASTNode {
    std::vector<ObjectPair> pairs;
};

/* Property access expression: obj.name */
struct PropertyAccessNode : ASTNode {
    std::unique_ptr<ASTNode> object;
    std::string              property;
};

/* Index access expression: arr[expr] */
struct IndexExprNode : ASTNode {
    std::unique_ptr<ASTNode> object;
    std::unique_ptr<ASTNode> index;
};

/*
 * Assignment to a property or array element used as a statement:
 *   obj.name = value
 *   arr[0]   = value
 *   arr[0].name = value   (chained)
 */
struct LValueAssignStatement : ASTNode {
    std::unique_ptr<ASTNode> target; // PropertyAccessNode or IndexExprNode
    std::unique_ptr<ASTNode> value;
};

/* The top-level program: an ordered list of statements. */
struct Program {
    std::vector<std::unique_ptr<ASTNode>> statements;
};