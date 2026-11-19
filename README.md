# Scheme Interpreter (C++)

A C++17 implementation of an educational Scheme-like interpreter, developed during the **Spring 2025 semester (junior year, second semester)** as a university programming languages course project.

**Original development period:** First half of 2025. This repository was organized for portfolio publication in November 2026; the upload date is not the development date.

## Features

- Lexical analysis and S-expression parsing
- Expression evaluation, variable binding, and environments
- `define`, `quote`, `lambda`, `let`, `if`, `cond`, `begin`
- Arithmetic and list operations
- Error reporting and a console REPL

> This is an educational **Scheme subset**, not a complete standards-compliant Scheme interpreter.

## Project structure

```text
src/main.cpp                 Current main version (original Project3.cpp)
tests/smoke-test.in          Example console inputs
archive/early-versions/      Earlier C++ iterations
archive/experiments/         Java, JavaScript and C experiments
```

The exact duplicate `test.cpp` was not copied. Compiled `.exe` files and editor settings are excluded from the GitHub-ready package. The original ZIP remains unchanged.

## Build and run

Requires a C++17-compatible compiler.

```bash
g++ -std=c++17 -O2 src/main.cpp -o scheme
./scheme
```

Windows PowerShell (MinGW g++):

```powershell
g++ -std=c++17 -O2 src/main.cpp -o scheme.exe
.\scheme.exe
```

**Input note:** The original program reads and discards an initial line before starting the REPL. Enter `1` as the first input line, then type Scheme expressions. This behavior has been preserved rather than changed without additional regression testing.

Sample session:

```text
1
(+ 1 (* 2 3))
(define x 10)
(+ x 2)
((lambda (x) (+ x 1)) 4)
(exit)
```

Expected evaluation outputs: `7`, `x defined`, `12`, and `5`.

To run with the sample file:

```bash
./scheme < tests/smoke-test.in
```

## Implementation notes

The main file integrates lexical analysis, parsing into S-expressions, evaluation with an environment, and formatting of results. The archived iterations document the evolution of the course implementation.

## Verification status

- `src/main.cpp` compiled successfully with g++ using C++17.
- The sample expressions above were executed successfully.
- This is a smoke test, **not** a comprehensive correctness or security audit.

## Credit

This repository preserves a student programming-languages project. Before publishing, verify academic course policies and appropriate attribution for any starter code.
