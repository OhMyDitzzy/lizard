#pragma once
#include <string>
#include <unordered_map>
#include "value.hpp"

struct Variable {
    LizardValue value;
    bool        is_mutable;
};

/*
 * Environment holds all variables in the current scope.
 *
 * Right now there is only one flat global scope. When functions
 * and blocks are added, each will push a new Environment that holds
 * a pointer to its enclosing one, and get/assign will walk the chain.
 */
class Environment {
public:
    void define(const std::string& name,
                LizardValue value,
                bool is_mutable,
                const std::string& filepath,
                const std::string& source,
                int line, int col);

    LizardValue get(const std::string& name,
                    const std::string& filepath,
                    const std::string& source,
                    int line, int col) const;

    void assign(const std::string& name,
                LizardValue value,
                const std::string& filepath,
                const std::string& source,
                int line, int col);

private:
    std::unordered_map<std::string, Variable> vars_;
};