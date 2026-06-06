<div align="center">
  <img src="assets/logo.png" alt="Lizard Logo" width="160"/>

  <h1>Lizard</h1>

  <p><strong>An expressive scripting language built in C++ — designed to grow with you.</strong></p>

  [![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](LICENSE)
  [![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
  [![CMake](https://img.shields.io/badge/CMake-3.20+-blue.svg)](https://cmake.org/)
  [![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)]()

</div>

---

## What is Lizard?

Lizard is a general-purpose, interpreted scripting language with clean syntax and modern semantics. Files use the `.lz` extension. The interpreter is written in C++17 with a hand-written lexer, recursive-descent parser, and tree-walking interpreter.

Lizard is an active work in progress. Core language features are stable and usable today, with more on the way.

## Quick Look

```lz
// Variables — let is immutable binding, var is mutable
let name = "Lizard"
var score int = 0

// Format strings
show(f#"Hello from {name}!")

// Arrays and Objects
let fruits = ["apple", "banana", "mango"]
let user   = #{ name: "Alice", age: 25 }

show(fruits[0])      // apple
show(user.name)      // Alice

// Mutate through a let binding — the binding is fixed, the contents are not
user.age = 26
fruits[0] = "grape"

// Ternary and nullish coalescing
let label  = score > 90 ? "A" : "B"
let config = null ?? "default"

// if / elif / else  — no outer parentheses required
if score >= 90 {
    show("Excellent!")
} elif score >= 70 {
    show("Good")
} else {
    show("Keep going")
}
```

## Features

| Feature | Status |
|---|---|
| Variables (`let` / `var`) with type hints | ✅ |
| Primitive types: `int`, `float`, `string`, `bool`, `null` | ✅ |
| Format strings `f#"Hello {expr}"` | ✅ |
| Single, double, and backtick (multiline) string literals | ✅ |
| Arithmetic, bitwise, logical, comparison operators | ✅ |
| Ternary `? :` and nullish coalescing `??` | ✅ |
| `if` / `elif` / `else` with optional `{}` or indentation | ✅ |
| Arrays `[]` with index access and mutation | ✅ |
| Objects `#{}` with property access and mutation | ✅ |
| Reference semantics for arrays and objects | ✅ |
| Rust-style error messages with source location | ✅ |
| Debug build mode with token + execution trace | ✅ |
| Functions and return values | 🔜 |
| Loops (`while`, `for`) | 🔜 |
| Standard library | 🔜 |
| Modules and imports | 🔜 |
| And many others | 🔜 |
 
## Building from Source

**Requirements:** CMake 3.20+, a C++17-capable compiler (GCC 9+, Clang 10+, MSVC 2019+)

```bash
# Clone
git clone https://github.com/OhMyDitzzy/lizard.git
cd lizard

# Release build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Debug build (enables [DEBUG] output)
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

The binary is placed at `build/lizard` (or `build-debug/lizard`).

## Running a Lizard file

```bash
./build/lizard path/to/script.lz
```

Try the included examples:

```bash
./build/lizard examples/hello.lz
./build/lizard examples/test_if.lz
```

## Language Overview

### Variables

```lz
let x = 42           // immutable binding, type inferred
let y float = 3.14   // explicit type hint (not enforced at runtime)
var count = 0        // mutable
var name             // uninitialized → null
var total int        // uninitialized with type → 0 (zero value)
```

### Strings

```lz
let a = "double quoted"
let b = 'single quoted'
let c = `
  multiline
  backtick string
`
let d = f#"interpolated: {a}, math: {1 + 2}"
```

### Arrays

```lz
let nums = [1, 2, 3, 4, 5]
show(nums[0])      // 1
show(nums[-1])     // 5  (negative indexing)
nums[0] = 99
```

### Objects

```lz
let person = #{ name: "Budi", age: 19 }
show(person.name)       // Budi
person.age  = 20        // update existing property
person.city = "Jakarta" // add new property
show(person)            // { name: "Budi", age: 20, city: "Jakarta" }
```

### Operators

```lz
// Arithmetic
show(10 / 3)    // 3.333333333
show(10 / 2)    // 5  (whole result → integer)
show(7 % 3)     // 1

// Bitwise
show(5 & 3)   // 1
show(5 | 3)   // 7
show(5 ^ 3)   // 6
show(2 << 3)  // 16

// Logical (both symbol and word form work)
show(true && false)   // false
show(true and false)  // false
show(!true)           // false
show(not true)        // false
```

### Control Flow

```lz
// Same-line
if x > 5 show("big")

// Indentation style (last statement ends with ;)
if x > 5
    show("big")
    show("very big");

// Brace style (recommended for nesting)
if x > 10 {
    show("very big")
} elif x > 5 {
    show("big")
} else {
    show("small")
}
```

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.

## License

Lizard is licensed under the [Apache 2.0 License](LICENSE).