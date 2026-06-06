# Changelog

All notable changes to Lizard will be documented here.

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).  
Lizard does not follow semantic versioning yet — versions will be added once the language stabilises.

---

## [Unreleased]

### Added
- Variables: `let` (immutable binding) and `var` (mutable) with optional type hints
- Primitive types: `int`, `float`, `string`, `bool`, `null`
- Zero-value initialisation for typed `var` declarations (`int` → 0, `string` → "", etc.)
- Untyped `var` without initialiser defaults to `null`
- String literals: double-quoted, single-quoted, and backtick multiline
- Escape sequences: `\n`, `\t`, `\r`, `\\`, `\"`, `\'`, `\e`, `\0`, `\xHH`, `\NNN` (octal)
- Format strings with `f#"..."`, `f#'...'`, and `` f#`...` `` (multiline)
- Arithmetic operators: `+`, `-`, `*`, `/`, `%`
- Bitwise operators: `&`, `|`, `^`, `~`, `<<`, `>>`
- Comparison operators: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical operators: `&&`/`and`, `||`/`or`, `!`/`not`
- Ternary expression: `condition ? then : else`
- Nullish coalescing: `value ?? fallback`
- Smart numeric results: whole-number results display as `int`, fractional as `float`
- `if` / `elif` / `else` with three block styles: same-line, indentation+`;`, and `{}`
- Arrays `[...]` with index access (`arr[0]`), negative indices, and element mutation
- Objects `#{ key: value }` with property access (`obj.key`) and property mutation
- Reference semantics for arrays and objects
- Built-in `show()` with `end_ln`, `start_ln`, and `sep` named arguments
- Rust-style error messages with file path, line, column, source snippet, and optional tip
- Debug build mode (`LIZARD_DEBUG`) with token stream and execution trace

[Unreleased]: https://github.com/OhMyDitzzy/lizard/compare/HEAD