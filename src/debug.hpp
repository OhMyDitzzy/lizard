#pragma once

/*
 * LZ_DEBUG(expr) writes a debug line to stderr during development.
 * It compiles away to nothing in Release builds so there is zero
 * overhead and no debug noise for end users.
 *
 * Usage:
 *   LZ_DEBUG("executing show() at line " << node.line);
 */
#ifdef LIZARD_DEBUG
    #include <iostream>
    #define LZ_DEBUG(expr) (std::cerr << "[DEBUG] " << expr << "\n")
#else
    #define LZ_DEBUG(expr) ((void)0)
#endif