#include "error.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

/*
 * ANSI escape codes used in the diagnostic output.
 * These work on Unix/macOS and modern Windows Terminal.
 * Older Windows CMD will show raw escape characters; colors can
 * be made optional later if cross-platform support is needed.
 */
namespace ansi {
constexpr const char* RESET = "\033[0m";
constexpr const char* BOLD_RED = "\033[1;31m"; // error label and underline
constexpr const char* BOLD = "\033[1m";        // error message
constexpr const char* BLUE = "\033[1;34m";     // arrows and gutter bars
constexpr const char* CYAN = "\033[0;36m";     // tip label
} // namespace ansi

std::string getSourceLine(const std::string& source, int lineNumber) {
    std::istringstream stream(source);
    std::string line;
    int current = 1;

    while (std::getline(stream, line)) {
        if (current == lineNumber) {
            // Strip trailing \r so Windows-style CRLF endings don't
            // leave a stray character at the end of the printed line.
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }
        current++;
    }

    return "";
}

void reportError(const LizardError& err, const std::string& source) {
    std::string sourceLine = getSourceLine(source, err.line);

    std::string lineNumStr = std::to_string(err.line);
    int gw = (int)lineNumStr.size(); // gutter width matches line-number digit count

    /*
     * The underline is (col - 1) spaces followed by ^ repeated for
     * however many characters the offending token spans.
     */
    std::string underline(err.col - 1, ' ');
    underline += std::string(std::max(1, err.length), '^');

    std::cerr << "\n"
              << ansi::BOLD_RED << "error[" << err.code << "]: " << ansi::RESET << ansi::BOLD
              << err.message << ansi::RESET
              << "\n"

              // " --> filepath:line:col"  — indented by gutter width
              << std::string(gw, ' ') << ansi::BLUE << " --> " << ansi::RESET << err.filepath << ":"
              << err.line << ":" << err.col
              << "\n"

              // blank gutter bar
              << std::string(gw + 1, ' ') << ansi::BLUE << "|" << ansi::RESET
              << "\n"

              // the actual source line
              << lineNumStr << ansi::BLUE << " | " << ansi::RESET << sourceLine
              << "\n"

              // the underline pointing at the problem
              << std::string(gw + 1, ' ') << ansi::BLUE << "| " << ansi::RESET << ansi::BOLD_RED
              << underline << ansi::RESET
              << "\n"

              // closing blank gutter bar
              << std::string(gw + 1, ' ') << ansi::BLUE << "|" << ansi::RESET << "\n";

    if (!err.tip.empty()) {
        std::cerr << std::string(gw + 1, ' ') << ansi::CYAN << "= tip: " << ansi::RESET << err.tip
                  << "\n";
    }

    std::cerr << "\n";
}