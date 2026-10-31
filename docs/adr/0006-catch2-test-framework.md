# ADR-0006: Catch2 v3 test framework

Status: ACCEPTED (owner approval, Phase 4b gate)
Requirements: NFR-01, NFR-02, NFR-07

## Context
Tests need tolerance-based floating-point comparison (NFR-01), table-style reference cases, compile-fail checks for unit safety (NFR-02) and CLI tests.

## Options
1. **Catch2 v3:** `WithinRel` / `WithinAbs` matchers, sections, generators for parameter tables; integrates with CTest.
2. **GoogleTest:** very common; parameterised tests are more verbose; floating-point matchers exist but are less ergonomic.
3. **doctest:** lightest, fewer features.

## Decision (proposed)
Option 1 via FetchContent, pinned. Compile-fail tests use CMake `try_compile` or `WILL_FAIL` targets, independent of the framework.

## Consequences
- Each requirement ID appears in test names or tags, so the traceability matrix can be generated or checked.
- Tests run under `ctest`, with sanitizer builds (address, undefined) as an extra configuration for NFR-03.
