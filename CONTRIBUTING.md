# Contributing to Lizard

Thank you for your interest in contributing to Lizard! This document explains how to get involved, what we expect from contributors, and how the development workflow is structured.

## Table of Contents

- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Branch Strategy](#branch-strategy)
- [Making Changes](#making-changes)
- [Pull Request Process](#pull-request-process)
- [Code Style](#code-style)
- [Reporting Bugs](#reporting-bugs)
- [Requesting Features](#requesting-features)
- [What to Work On](#what-to-work-on)

---

## Getting Started

1. Fork the repository on GitHub
2. Clone your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/lizard.git
   cd lizard
   ```
3. Add the upstream remote:
   ```bash
   git remote add upstream https://github.com/OhMyDitzzy/lizard.git
   ```

---

## Development Setup

**Requirements:** CMake 3.20+, GCC 9+ / Clang 10+ / MSVC 2019+, C++17 support

```bash
# Debug build — enables LZ_DEBUG output and assertions
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# Run a test file
./build-debug/lizard examples/hello.lz
```

We recommend working with the Debug build during development so you can see the token stream and execution trace while debugging the interpreter.

---

## Branch Strategy

| Branch | Purpose |
|---|---|
| `main` | Stable, releasable code only |
| `feature/<name>` | New language features (e.g. `feature/while-loop`) |
| `fix/<name>` | Bug fixes (e.g. `fix/string-escape`) |
| `docs/<name>` | Documentation-only changes |
| `refactor/<name>` | Internal refactoring with no behavior change |

Always branch off `main`:

```bash
git checkout main
git pull upstream main
git checkout -b feature/your-feature-name
```

---

## Making Changes

### Language features

Lizard's source is layered: **Lexer → Parser → AST → Interpreter**. A typical feature touches:

1. `src/core/token.hpp` — add new token type(s)
2. `src/core/lexer.cpp` — lex the new syntax
3. `src/core/ast.hpp` — add AST node(s)
4. `src/core/parser.hpp / parser.cpp` — parse into the AST
5. `src/core/interpreter.hpp / interpreter.cpp` — evaluate the AST

Built-in functions go in `src/builtins/<category>/`.

### Comments

Write comments only where the code is non-obvious. Prefer clear naming over explanatory comments. Do **not** use decorative banners like `// ─── helpers ───`.

Good:
```cpp
// Returns the zero value for a known type hint.
// Called when 'var x type' is declared without an initializer.
static LizardValue zeroValueFor(const std::string& type_hint, ...);
```

Avoid:
```cpp
// ─── helpers ──────────────────────────────────────────────
```

### Error messages

New error codes follow the existing `E0xx` scheme. Every error must include:
- A clear, lowercase message describing what went wrong
- A _Opsional_ `tip:` where it helps the user fix the issue

---

## Pull Request Process

1. Make sure the project builds cleanly in both Debug and Release:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
   cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build-debug
   ```
2. Add or update `.lz` example files in `examples/` if your change affects language behavior
3. Open a PR against `main` with a clear title and description:
   - **What** the change does
   - **Why** it is needed
   - Any design decisions or trade-offs made
4. Keep PRs focused — one feature or fix per PR makes review easier
5. Be responsive to review feedback

---

## Code Style

The project uses `.clang-format` at the repository root. Run it before committing:

```bash
# Format a single file
clang-format -i src/core/parser.cpp

# Format all source files
find src -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
```

Key points enforced by the style file:
- 4-space indentation, no tabs
- Opening braces on the same line
- Column limit of 100 characters
- Left-aligned pointer/reference qualifiers (`const std::string& s`)

---

## Reporting Bugs

Open an issue and include:

1. **Lizard version / commit hash**
2. **The `.lz` file or snippet** that triggers the bug
3. **Expected behavior**
4. **Actual output** (include the full error message if applicable)
5. **Platform** (OS, compiler, CMake version)

---

## Requesting Features

Open an issue with the **Feature Request** template. Describe:

1. **The problem** you want to solve, not just the solution
2. **Proposed syntax** if applicable — compare with existing Lizard conventions
3. **Alternatives considered**

New syntax decisions are discussed in the issue before implementation begins, so we can reach agreement on the design first.

---

## What to Work On

Check the [open issues](https://github.com/OhMyDitzzy/lizard/issues) for things labeled:

- `good first issue` — self-contained, well-scoped tasks
- `help wanted` — higher complexity but community input is welcome
- `language design` — syntax/semantics discussions that need broader input

---

By contributing to Lizard, you agree that your contributions will be licensed under the [Apache 2.0 License](LICENSE).