#include "core/interpreter.hpp"
#include "core/lexer.hpp"
#include "core/parser.hpp"
#include "error/error.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef LIZARD_DEBUG
    #include "core/token.hpp"
    #include <iomanip>

/*
 * Print the full token stream produced by the lexer.
 * Only compiled and called in Debug builds — helps verify that the
 * lexer is splitting source text the way we expect before chasing
 * bugs in the parser or interpreter.
 */
static void debugDumpTokens(const std::vector<Token>& tokens) {
    std::cerr << "[DEBUG] Token stream (" << tokens.size() << " tokens):\n";
    for (const auto& t : tokens) {
        std::cerr << "  " << std::setw(4) << t.line << ":" << std::setw(3) << t.col << "  "
                  << "type=" << std::setw(3) << static_cast<int>(t.type) << "  "
                  << "value='" << t.value << "'\n";
    }
    std::cerr << "\n";
}
#endif

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: lizard <file.lz>\n";
        return 1;
    }

    std::string filepath = argv[1];
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "error: cannot open file '" << filepath << "'\n";
        return 1;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    Lexer lexer(filepath, source);
    auto tokens = lexer.tokenize();

#ifdef LIZARD_DEBUG
    debugDumpTokens(tokens);
#endif

    Parser parser(filepath, source, tokens);
    Program program = parser.parse();

    Interpreter interpreter(filepath, source);
    interpreter.execute(program);

    return 0;
}