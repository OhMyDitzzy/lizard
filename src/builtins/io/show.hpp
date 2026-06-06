#pragma once
#include <string>
#include <vector>

/*
 * Arguments collected by the interpreter before calling builtin_show().
 * All three named arguments have sensible defaults so the common
 * case show("hello") just works.
 */
struct ShowArgs {
    std::vector<std::string> values; // positional values in source order
    std::string start_ln = "";       // printed once before all values
    std::string end_ln = "\n";       // printed once after all values
    std::string sep = " ";           // inserted between consecutive values
};

/* Write the show() output to stdout according to ShowArgs. */
void builtin_show(const ShowArgs& args);