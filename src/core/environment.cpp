#include "environment.hpp"
#include "../error/error.hpp"
#include <cstdlib>

void Environment::define(const std::string& name, LizardValue value, bool is_mutable,
                         const std::string& filepath, const std::string& source, int line,
                         int col) {
    if (vars_.count(name)) {
        LizardError err;
        err.code = "E032";
        err.message = "variable '" + name + "' is already defined in this scope";
        err.filepath = filepath;
        err.line = line;
        err.col = col;
        err.length = static_cast<int>(name.size());
        reportError(err, source);
        std::exit(1);
    }

    vars_[name] = Variable{value, is_mutable};
}

LizardValue Environment::get(const std::string& name, const std::string& filepath,
                             const std::string& source, int line, int col) const {
    auto it = vars_.find(name);

    if (it == vars_.end()) {
        LizardError err;
        err.code = "E030";
        err.message = "undefined variable '" + name + "'";
        err.filepath = filepath;
        err.line = line;
        err.col = col;
        err.length = static_cast<int>(name.size());
        err.tip = "declare it first with 'let " + name + " = ...' or 'var " + name + " = ...'";
        reportError(err, source);
        std::exit(1);
    }

    return it->second.value;
}

void Environment::assign(const std::string& name, LizardValue value, const std::string& filepath,
                         const std::string& source, int line, int col) {
    auto it = vars_.find(name);

    if (it == vars_.end()) {
        LizardError err;
        err.code = "E030";
        err.message = "undefined variable '" + name + "'";
        err.filepath = filepath;
        err.line = line;
        err.col = col;
        err.length = static_cast<int>(name.size());
        err.tip = "declare it first with 'var " + name + " = ...'";
        reportError(err, source);
        std::exit(1);
    }

    if (!it->second.is_mutable) {
        LizardError err;
        err.code = "E031";
        err.message = "cannot reassign immutable variable '" + name + "'";
        err.filepath = filepath;
        err.line = line;
        err.col = col;
        err.length = static_cast<int>(name.size());
        err.tip = "use 'var' instead of 'let' if you need to reassign this variable";
        reportError(err, source);
        std::exit(1);
    }

    it->second.value = value;
}