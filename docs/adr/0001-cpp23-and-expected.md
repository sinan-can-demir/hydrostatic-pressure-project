# ADR-0001: C++23 and std::expected for error handling

Status: ACCEPTED (owner approval, Phase 4b gate)
Requirements: NFR-04, NFR-05, CON-01

## Context
The public API must not throw (NFR-04) and must return either a result or a list of errors. The project currently sets C++20.

## Options
1. **C++23 with `std::expected`.** Standard, no dependency. Needs GCC 12+ / Clang 16+ with a matching standard library.
2. **C++20 with `tl::expected` or a hand-written `Expected`.** Works on older compilers; adds a dependency or code to maintain; later migration to the standard type.
3. **C++20 with exceptions internally, translated at the boundary.** Violates the spirit of NFR-04 and makes error paths harder to test.

## Decision (proposed)
Option 1. The installed toolchain is GCC 16, and the project has no legacy compiler constraint. CON-01 and NFR-05 are amended to C++23.

## Consequences
- Simple, dependency-free error type; monadic operations (`and_then`) are available.
- Clang must be version 16+ with libstdc++ 12+ or libc++ 17+ for the portability check.
- `CMakeLists.txt` moves to `CMAKE_CXX_STANDARD 23`.
