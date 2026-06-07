#include "interpreter.hpp"
#include "../builtins/io/show.hpp"
#include "../debug.hpp"
#include "../error/error.hpp"
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>

Interpreter::Interpreter(const std::string& filepath, const std::string& source)
    : filepath_(filepath), source_(source) {}

void Interpreter::execute(const Program& program) {
    for (const auto& stmt : program.statements)
        executeStatement(*stmt);
}

void Interpreter::executeStatement(const ASTNode& node) {
    if (const auto* s = dynamic_cast<const ShowStatement*>(&node)) {
        executeShow(*s);
        return;
    }
    if (const auto* s = dynamic_cast<const VarDeclStatement*>(&node)) {
        executeVarDecl(*s);
        return;
    }
    if (const auto* s = dynamic_cast<const AssignStatement*>(&node)) {
        executeAssign(*s);
        return;
    }
    if (const auto* s = dynamic_cast<const IfStatement*>(&node)) {
        executeIf(*s);
        return;
    }
    if (const auto* s = dynamic_cast<const LValueAssignStatement*>(&node)) {
        executeLValueAssign(*s);
        return;
    }
    if (const auto* s = dynamic_cast<const CompoundAssignStatement*>(&node)) {
        executeCompoundAssign(*s);
        return;
    }

    LizardError err;
    err.code = "E020";
    err.message = "interpreter encountered an unknown statement type";
    err.filepath = filepath_;
    err.line = node.line;
    err.col = node.col;
    err.length = 1;
    reportError(err, source_);
    std::exit(1);
}

static LizardValue zeroValueFor(const std::string& type_hint, const std::string& filepath,
                                const std::string& source, int line, int col) {
    if (type_hint == "int") return LizardValue::makeInt("0");
    if (type_hint == "float") return LizardValue::makeFloat("0.0");
    if (type_hint == "string") return LizardValue::makeString("");
    if (type_hint == "bool") return LizardValue::makeBool(false);
    if (type_hint == "null") return LizardValue::makeNull();

    LizardError err;
    err.code = "E033";
    err.message = "unknown type '" + type_hint + "', cannot determine zero value";
    err.filepath = filepath;
    err.line = line;
    err.col = col;
    err.length = static_cast<int>(type_hint.size());
    err.tip = "types with defined zero values are: int, float, string, bool";
    reportError(err, source);
    std::exit(1);
}

void Interpreter::executeVarDecl(const VarDeclStatement& node) {
    LZ_DEBUG((node.is_mutable ? "var" : "let") << " '" << node.name << "' at line " << node.line);

    LizardValue value;
    if (node.initializer) {
        value = evalValue(*node.initializer);
    } else {
        if (node.type_hint.empty()) {
            // No type hint and no initializer — default to null.
            value = LizardValue::makeNull();
        } else {
            value = zeroValueFor(node.type_hint, filepath_, source_, node.line, node.col);
        }
    }

    env_.define(node.name, value, node.is_mutable, filepath_, source_, node.line, node.col);
}

void Interpreter::executeAssign(const AssignStatement& node) {
    LZ_DEBUG("assign '" << node.name << "' at line " << node.line);
    env_.assign(node.name, evalValue(*node.value), filepath_, source_, node.line, node.col);
}

void Interpreter::executeShow(const ShowStatement& node) {
    ShowArgs args;
    for (const auto& arg : node.args)
        args.values.push_back(evalValue(*arg).display());

    auto applyNamed = [&](const std::string& name, std::string& target) {
        auto it = node.named_args.find(name);
        if (it != node.named_args.end()) target = evalValue(*it->second).display();
    };
    applyNamed("end_ln", args.end_ln);
    applyNamed("start_ln", args.start_ln);
    applyNamed("sep", args.sep);
    builtin_show(args);
}

LizardValue Interpreter::evalValue(const ASTNode& node) {
    if (const auto* lit = dynamic_cast<const LiteralNode*>(&node)) {
        switch (lit->kind) {
            case LiteralNode::Kind::String:
                return LizardValue::makeString(lit->value);
            case LiteralNode::Kind::Integer:
                return LizardValue::makeInt(lit->value);
            case LiteralNode::Kind::Float:
                return LizardValue::makeFloat(lit->value);
            case LiteralNode::Kind::Bool:
                return LizardValue::makeBool(lit->value == "true");
            case LiteralNode::Kind::Null:
                return LizardValue::makeNull();
        }
    }
    if (const auto* ident = dynamic_cast<const IdentifierNode*>(&node))
        return env_.get(ident->name, filepath_, source_, ident->line, ident->col);

    if (const auto* arr = dynamic_cast<const ArrayLiteralNode*>(&node)) {
        auto data = std::make_shared<LizardArray>();
        for (const auto& el : arr->elements)
            data->elements.push_back(evalValue(*el));
        return LizardValue::makeArray(data);
    }

    if (const auto* obj = dynamic_cast<const ObjectLiteralNode*>(&node)) {
        auto data = std::make_shared<LizardObject>();
        for (const auto& pair : obj->pairs)
            data->set(pair.key, evalValue(*pair.value));
        return LizardValue::makeObject(data);
    }

    if (const auto* prop = dynamic_cast<const PropertyAccessNode*>(&node))
        return evalPropertyAccess(*prop);

    if (const auto* idx = dynamic_cast<const IndexExprNode*>(&node)) return evalIndexAccess(*idx);

    if (const auto* bin = dynamic_cast<const BinaryExprNode*>(&node)) {
        /* Short-circuit: &&, ||, and ?? don't always evaluate both sides. */
        if (bin->op == "&&" || bin->op == "||") {
            LizardValue lhs = evalValue(*bin->left);
            bool lhsTruthy = isTruthy(lhs);
            if (bin->op == "&&" && !lhsTruthy) return LizardValue::makeBool(false);
            if (bin->op == "||" && lhsTruthy) return LizardValue::makeBool(true);
            return LizardValue::makeBool(isTruthy(evalValue(*bin->right)));
        }
        if (bin->op == "??") {
            LizardValue lhs = evalValue(*bin->left);
            return lhs.kind != LizardValue::Kind::Null ? lhs : evalValue(*bin->right);
        }
        return evalBinary(*bin);
    }

    if (const auto* ternary = dynamic_cast<const TernaryExprNode*>(&node)) {
        return isTruthy(evalValue(*ternary->condition)) ? evalValue(*ternary->then_expr)
                                                        : evalValue(*ternary->else_expr);
    }

    if (const auto* unary = dynamic_cast<const UnaryExprNode*>(&node)) return evalUnary(*unary);

    LizardError err;
    err.code = "E021";
    err.message = "cannot evaluate this expression yet";
    err.filepath = filepath_;
    err.line = node.line;
    err.col = node.col;
    err.length = 1;
    reportError(err, source_);
    std::exit(1);
}

/*
 * Truthiness rules:
 *   null, false (0), 0, 0.0, ""  →  falsy
 *   everything else               →  truthy
 */
bool Interpreter::isTruthy(const LizardValue& v) {
    switch (v.kind) {
        case LizardValue::Kind::Null:
            return false;
        case LizardValue::Kind::Bool:
            return v.raw == "1";
        case LizardValue::Kind::Integer:
            return v.raw != "0";
        case LizardValue::Kind::Float:
            return std::stod(v.raw) != 0.0;
        case LizardValue::Kind::String:
            return !v.raw.empty();
        case LizardValue::Kind::Array:
            return !v.array_data->elements.empty();
        case LizardValue::Kind::Object:
            return !v.object_data->fields.empty();
    }
    return false;
}

static bool isNumeric(const LizardValue& v) {
    return v.kind == LizardValue::Kind::Integer || v.kind == LizardValue::Kind::Float ||
           v.kind == LizardValue::Kind::Bool;
}

static bool isIntLike(const LizardValue& v) {
    return v.kind == LizardValue::Kind::Integer || v.kind == LizardValue::Kind::Bool;
}

static double toDouble(const LizardValue& v) {
    if (v.kind == LizardValue::Kind::Bool) return v.raw == "1" ? 1.0 : 0.0;
    return std::stod(v.raw);
}

static long long toInt(const LizardValue& v) {
    if (v.kind == LizardValue::Kind::Bool) return v.raw == "1" ? 1LL : 0LL;
    return std::stoll(v.raw);
}

static std::string formatFloat(double d) {
    std::ostringstream oss;
    oss << std::setprecision(10) << d;
    std::string s = oss.str();
    auto dot = s.find('.');
    if (dot != std::string::npos) {
        auto last = s.find_last_not_of('0');
        s = s.substr(0, (last == dot ? dot + 2 : last + 1));
    }
    return s;
}

static LizardValue smartNumeric(double d) {
    if (std::floor(d) == d && d >= -9.2e18 && d <= 9.2e18)
        return LizardValue::makeInt(std::to_string(static_cast<long long>(d)));
    return LizardValue::makeFloat(formatFloat(d));
}

LizardValue Interpreter::evalComparison(const LizardValue& left, const LizardValue& right,
                                        const std::string& op, const BinaryExprNode& node) {
    /* Array and Object: reference equality — two vars are equal only if they share the same object.
     */
    if (left.kind == LizardValue::Kind::Array || left.kind == LizardValue::Kind::Object) {
        bool same =
            (left.kind == right.kind) && (left.kind == LizardValue::Kind::Array
                                              ? left.array_data.get() == right.array_data.get()
                                              : left.object_data.get() == right.object_data.get());
        if (op == "==") return LizardValue::makeBool(same);
        if (op == "!=") return LizardValue::makeBool(!same);

        LizardError err;
        err.code = "E044";
        err.message = "cannot order-compare arrays or objects with '" + op + "'";
        err.filepath = filepath_;
        err.line = node.line;
        err.col = node.col;
        err.length = static_cast<int>(op.size());
        err.tip = "arrays and objects support only '==' and '!='";
        reportError(err, source_);
        std::exit(1);
    }

    /* null comparisons */
    if (left.kind == LizardValue::Kind::Null || right.kind == LizardValue::Kind::Null) {
        bool bothNull =
            (left.kind == LizardValue::Kind::Null && right.kind == LizardValue::Kind::Null);
        if (op == "==") return LizardValue::makeBool(bothNull);
        if (op == "!=") return LizardValue::makeBool(!bothNull);

        LizardError err;
        err.code = "E044";
        err.message = "cannot order-compare null with '" + op + "'";
        err.filepath = filepath_;
        err.line = node.line;
        err.col = node.col;
        err.length = static_cast<int>(op.size());
        err.tip = "null supports only '==' and '!='";
        reportError(err, source_);
        std::exit(1);
    }

    /* Numeric comparison (int, float, bool all compare as double). */
    if (isNumeric(left) && isNumeric(right)) {
        double l = toDouble(left), r = toDouble(right);
        if (op == "==") return LizardValue::makeBool(l == r);
        if (op == "!=") return LizardValue::makeBool(l != r);
        if (op == "<") return LizardValue::makeBool(l < r);
        if (op == ">") return LizardValue::makeBool(l > r);
        if (op == "<=") return LizardValue::makeBool(l <= r);
        if (op == ">=") return LizardValue::makeBool(l >= r);
    }

    /* String comparison. */
    if (left.kind == LizardValue::Kind::String && right.kind == LizardValue::Kind::String) {
        if (op == "==") return LizardValue::makeBool(left.raw == right.raw);
        if (op == "!=") return LizardValue::makeBool(left.raw != right.raw);
        if (op == "<") return LizardValue::makeBool(left.raw < right.raw);
        if (op == ">") return LizardValue::makeBool(left.raw > right.raw);
        if (op == "<=") return LizardValue::makeBool(left.raw <= right.raw);
        if (op == ">=") return LizardValue::makeBool(left.raw >= right.raw);
    }

    /* Different types: equality is always false, ordering is an error. */
    if (op == "==") return LizardValue::makeBool(false);
    if (op == "!=") return LizardValue::makeBool(true);

    LizardError err;
    err.code = "E044";
    err.message = "cannot compare these types with '" + op + "'";
    err.filepath = filepath_;
    err.line = node.line;
    err.col = node.col;
    err.length = static_cast<int>(op.size());
    reportError(err, source_);
    std::exit(1);
}

LizardValue Interpreter::evalBinary(const BinaryExprNode& node) {
    LizardValue left = evalValue(*node.left);
    LizardValue right = evalValue(*node.right);
    const std::string& op = node.op;

    if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=")
        return evalComparison(left, right, op, node);

    /* String concatenation: '+' joins when either side is a string. */
    if (op == "+") {
        if (left.kind == LizardValue::Kind::String || right.kind == LizardValue::Kind::String)
            return LizardValue::makeString(left.display() + right.display());
    }

    /* Bitwise operators: integer and bool only. */
    bool isBitwise = (op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>");
    if (isBitwise) {
        if (!isIntLike(left) || !isIntLike(right)) {
            LizardError err;
            err.code = "E043";
            err.message = "operator '" + op + "' requires integer operands";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = static_cast<int>(op.size());
            err.tip = "bitwise operations are only valid on 'int' and 'bool' values";
            reportError(err, source_);
            std::exit(1);
        }
        long long l = toInt(left), r = toInt(right);
        if (op == "&") return LizardValue::makeInt(std::to_string(l & r));
        if (op == "|") return LizardValue::makeInt(std::to_string(l | r));
        if (op == "^") return LizardValue::makeInt(std::to_string(l ^ r));
        if (op == "<<") return LizardValue::makeInt(std::to_string(l << r));
        if (op == ">>") return LizardValue::makeInt(std::to_string(l >> r));
    }

    /* Modulo: integer only. */
    if (op == "%") {
        if (!isIntLike(left) || !isIntLike(right)) {
            LizardError err;
            err.code = "E040";
            err.message = "operator '%' requires integer operands";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = 1;
            reportError(err, source_);
            std::exit(1);
        }
        long long l = toInt(left), r = toInt(right);
        if (r == 0) {
            LizardError err;
            err.code = "E042";
            err.message = "modulo by zero";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = 1;
            reportError(err, source_);
            std::exit(1);
        }
        return LizardValue::makeInt(std::to_string(l % r));
    }

    /* Arithmetic: +, -, *, / */
    if (!isNumeric(left) || !isNumeric(right)) {
        LizardError err;
        err.code = "E040";
        err.message = "operator '" + op + "' cannot be applied to these types";
        err.filepath = filepath_;
        err.line = node.line;
        err.col = node.col;
        err.length = static_cast<int>(op.size());
        if (left.kind == LizardValue::Kind::String || right.kind == LizardValue::Kind::String)
            err.tip = "'+' is the only operator that works with strings (concatenation)";
        reportError(err, source_);
        std::exit(1);
    }

    double l = toDouble(left), r = toDouble(right);
    if (op == "+") return smartNumeric(l + r);
    if (op == "-") return smartNumeric(l - r);
    if (op == "*") return smartNumeric(l * r);
    if (op == "/") {
        if (r == 0.0) {
            LizardError err;
            err.code = "E041";
            err.message = "division by zero";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = 1;
            reportError(err, source_);
            std::exit(1);
        }
        return smartNumeric(l / r);
    }

    return LizardValue::makeInt("0");
}

LizardValue Interpreter::evalUnary(const UnaryExprNode& node) {
    LizardValue operand = evalValue(*node.operand);

    if (node.op == "!" || node.op == "not") return LizardValue::makeBool(!isTruthy(operand));

    if (node.op == "-") {
        if (!isNumeric(operand)) {
            LizardError err;
            err.code = "E040";
            err.message = "unary '-' cannot be applied to a non-numeric value";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = 1;
            reportError(err, source_);
            std::exit(1);
        }
        return smartNumeric(-toDouble(operand));
    }

    if (node.op == "~") {
        if (!isIntLike(operand)) {
            LizardError err;
            err.code = "E043";
            err.message = "bitwise NOT '~' requires an integer operand";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = 1;
            err.tip = "bitwise operations are only valid on 'int' and 'bool' values";
            reportError(err, source_);
            std::exit(1);
        }
        return LizardValue::makeInt(std::to_string(~toInt(operand)));
    }

    return operand;
}

void Interpreter::executeIf(const IfStatement& node) {
    LZ_DEBUG("if at line " << node.line);

    if (isTruthy(evalValue(*node.condition))) {
        for (const auto& stmt : node.then_body)
            executeStatement(*stmt);
        return;
    }

    for (const auto& clause : node.elif_clauses) {
        if (isTruthy(evalValue(*clause.condition))) {
            for (const auto& stmt : clause.body)
                executeStatement(*stmt);
            return;
        }
    }

    for (const auto& stmt : node.else_body)
        executeStatement(*stmt);
}

LizardValue Interpreter::evalPropertyAccess(const PropertyAccessNode& node) {
    LizardValue obj = evalValue(*node.object);

    if (obj.kind != LizardValue::Kind::Object) {
        LizardError err;
        err.code = "E050";
        err.message = "cannot access property '" + node.property + "' on a non-object value";
        err.filepath = filepath_;
        err.line = node.line;
        err.col = node.col;
        err.length = static_cast<int>(node.property.size());
        err.tip = "property access with '.' is only valid on object values created with #{}";
        reportError(err, source_);
        std::exit(1);
    }

    return obj.object_data->get(node.property); // returns null if key is missing
}

LizardValue Interpreter::evalIndexAccess(const IndexExprNode& node) {
    LizardValue obj = evalValue(*node.object);
    LizardValue idx = evalValue(*node.index);

    if (obj.kind == LizardValue::Kind::Array) {
        if (idx.kind != LizardValue::Kind::Integer && idx.kind != LizardValue::Kind::Bool) {
            LizardError err;
            err.code = "E051";
            err.message = "array index must be an integer";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = 1;
            reportError(err, source_);
            std::exit(1);
        }
        long long i = toInt(idx);
        auto& elements = obj.array_data->elements;

        // Support negative indices: -1 is the last element
        if (i < 0) i = static_cast<long long>(elements.size()) + i;

        if (i < 0 || i >= static_cast<long long>(elements.size())) {
            LizardError err;
            err.code = "E052";
            err.message = "array index " + std::to_string(i) + " is out of bounds";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = 1;
            err.tip = "array has " + std::to_string(elements.size()) +
                      " element(s), valid indices are 0 to " +
                      std::to_string(static_cast<long long>(elements.size()) - 1);
            reportError(err, source_);
            std::exit(1);
        }
        return elements[static_cast<size_t>(i)];
    }

    if (obj.kind == LizardValue::Kind::Object) {
        return obj.object_data->get(idx.display());
    }

    LizardError err;
    err.code = "E053";
    err.message = "cannot index into a non-array/non-object value";
    err.filepath = filepath_;
    err.line = node.line;
    err.col = node.col;
    err.length = 1;
    reportError(err, source_);
    std::exit(1);
}

void Interpreter::executeLValueAssign(const LValueAssignStatement& node) {
    writeLValue(*node.target, evalValue(*node.value));
}

LizardValue Interpreter::readLValue(const ASTNode& target) {
    if (const auto* ident = dynamic_cast<const IdentifierNode*>(&target))
        return env_.get(ident->name, filepath_, source_, ident->line, ident->col);
    if (const auto* prop = dynamic_cast<const PropertyAccessNode*>(&target))
        return evalPropertyAccess(*prop);
    if (const auto* idx = dynamic_cast<const IndexExprNode*>(&target)) return evalIndexAccess(*idx);

    LizardError err;
    err.code = "E055";
    err.message = "invalid assignment target";
    err.filepath = filepath_;
    err.line = target.line;
    err.col = target.col;
    err.length = 1;
    reportError(err, source_);
    std::exit(1);
}

void Interpreter::writeLValue(const ASTNode& target, LizardValue val) {
    if (const auto* ident = dynamic_cast<const IdentifierNode*>(&target)) {
        env_.assign(ident->name, val, filepath_, source_, ident->line, ident->col);
        return;
    }
    if (const auto* prop = dynamic_cast<const PropertyAccessNode*>(&target)) {
        LizardValue obj = evalValue(*prop->object);
        if (obj.kind != LizardValue::Kind::Object) {
            LizardError err;
            err.code = "E050";
            err.message = "cannot set property '" + prop->property + "' on a non-object";
            err.filepath = filepath_;
            err.line = prop->line;
            err.col = prop->col;
            err.length = static_cast<int>(prop->property.size());
            reportError(err, source_);
            std::exit(1);
        }
        obj.object_data->set(prop->property, std::move(val));
        return;
    }
    if (const auto* idx = dynamic_cast<const IndexExprNode*>(&target)) {
        LizardValue obj = evalValue(*idx->object);
        LizardValue index = evalValue(*idx->index);
        if (obj.kind == LizardValue::Kind::Array) {
            long long i = toInt(index);
            auto& els = obj.array_data->elements;
            if (i < 0) i = static_cast<long long>(els.size()) + i;
            if (i < 0 || i >= static_cast<long long>(els.size())) {
                LizardError err;
                err.code = "E052";
                err.message = "array index " + std::to_string(i) + " out of bounds";
                err.filepath = filepath_;
                err.line = idx->line;
                err.col = idx->col;
                err.length = 1;
                reportError(err, source_);
                std::exit(1);
            }
            els[static_cast<size_t>(i)] = std::move(val);
            return;
        }
        if (obj.kind == LizardValue::Kind::Object) {
            obj.object_data->set(index.display(), std::move(val));
            return;
        }
        LizardError err;
        err.code = "E053";
        err.message = "cannot index into this value";
        err.filepath = filepath_;
        err.line = idx->line;
        err.col = idx->col;
        err.length = 1;
        reportError(err, source_);
        std::exit(1);
    }
}

void Interpreter::executeCompoundAssign(const CompoundAssignStatement& node) {
    LizardValue cur = readLValue(*node.target);
    LizardValue rhs = evalValue(*node.value);
    const std::string& op = node.op;
    LizardValue result;

    // String concatenation via +=
    if (op == "+" &&
        (cur.kind == LizardValue::Kind::String || rhs.kind == LizardValue::Kind::String)) {
        result = LizardValue::makeString(cur.display() + rhs.display());
    }
    // Bitwise compound assigns
    else if (op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>") {
        if (!isIntLike(cur) || !isIntLike(rhs)) {
            LizardError err;
            err.code = "E043";
            err.message = "operator '" + op + "=' requires integer operands";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = static_cast<int>(op.size()) + 1;
            err.tip = "bitwise operations are only valid on 'int' and 'bool' values";
            reportError(err, source_);
            std::exit(1);
        }
        long long l = toInt(cur), r = toInt(rhs);
        if (op == "&")
            result = LizardValue::makeInt(std::to_string(l & r));
        else if (op == "|")
            result = LizardValue::makeInt(std::to_string(l | r));
        else if (op == "^")
            result = LizardValue::makeInt(std::to_string(l ^ r));
        else if (op == "<<")
            result = LizardValue::makeInt(std::to_string(l << r));
        else
            result = LizardValue::makeInt(std::to_string(l >> r));
    }
    // Modulo
    else if (op == "%") {
        if (!isIntLike(cur) || !isIntLike(rhs)) {
            LizardError err;
            err.code = "E040";
            err.message = "operator '%=' requires integer operands";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = 2;
            reportError(err, source_);
            std::exit(1);
        }
        long long l = toInt(cur), r = toInt(rhs);
        if (r == 0) {
            LizardError err;
            err.code = "E042";
            err.message = "modulo by zero";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = 2;
            reportError(err, source_);
            std::exit(1);
        }
        result = LizardValue::makeInt(std::to_string(l % r));
    }
    // Arithmetic: +, -, *, /
    else {
        if (!isNumeric(cur) || !isNumeric(rhs)) {
            LizardError err;
            err.code = "E040";
            err.message = "operator '" + op + "=' cannot be applied to these types";
            err.filepath = filepath_;
            err.line = node.line;
            err.col = node.col;
            err.length = static_cast<int>(op.size()) + 1;
            reportError(err, source_);
            std::exit(1);
        }
        double l = toDouble(cur), r = toDouble(rhs);
        if (op == "+")
            result = smartNumeric(l + r);
        else if (op == "-")
            result = smartNumeric(l - r);
        else if (op == "*")
            result = smartNumeric(l * r);
        else if (op == "/") {
            if (r == 0.0) {
                LizardError err;
                err.code = "E041";
                err.message = "division by zero";
                err.filepath = filepath_;
                err.line = node.line;
                err.col = node.col;
                err.length = 2;
                reportError(err, source_);
                std::exit(1);
            }
            result = smartNumeric(l / r);
        }
    }

    writeLValue(*node.target, std::move(result));
}