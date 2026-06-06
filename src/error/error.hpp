#pragma once
#include <string>

/*
 * LizardError holds all the information needed to print
 * diagnostic: the error code, the human-readable message, where in
 * the source it happened, how many characters to underline, and an
 * optional tip that appears when there is something actionable to say.
 */
struct LizardError {
    std::string code;      // e.g. "E002"
    std::string message;   // e.g. "unexpected character '@'"
    std::string filepath;  // path to the .lz file
    int line   = 0;        // 1-based line number in the source
    int col    = 0;        // 1-based column number in the source
    int length = 1;        // number of source characters to underline with ^
    std::string tip;       // optional hint shown below the diagnostic
};

/* Print a error to stderr and the relevant source snippet. */
void reportError(const LizardError& err, const std::string& source);

/* Return the text of one line from source (line number is 1-based). */
std::string getSourceLine(const std::string& source, int lineNumber);