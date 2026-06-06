#include "show.hpp"
#include <iostream>

/*
 * show() is Lizard's built-in output function.
 *
 * It writes start_ln, then each positional value separated by sep,
 * then end_ln to stdout. Flushing is implicit via std::endl when
 * end_ln contains a newline, and explicit otherwise so that
 * show("Loading...", end_ln: "") appears on screen immediately.
 */
void builtin_show(const ShowArgs& args) {
    std::cout << args.start_ln;

    for (size_t i = 0; i < args.values.size(); i++) {
        if (i > 0) std::cout << args.sep;
        std::cout << args.values[i];
    }

    std::cout << args.end_ln;

    // Flush when end_ln does not contain a newline so partial lines
    // (e.g. prompts written with end_ln: "") appear immediately.
    bool endsWithNewline = !args.end_ln.empty() && args.end_ln.back() == '\n';
    if (!endsWithNewline) {
        std::cout.flush();
    }
}