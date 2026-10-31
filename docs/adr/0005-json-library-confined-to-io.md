# ADR-0005: nlohmann/json, confined to hp_io

Status: ACCEPTED (owner approval, Phase 4b gate)
Requirements: FR-17, FR-19, NFR-06, CON-01

## Context
The CLI reads and writes JSON. The core must stay free of third-party dependencies and I/O.

## Options
1. **nlohmann/json** via CMake FetchContent: header-only, ubiquitous, good error messages; needs network on first configure (or a vendored copy).
2. **Boost.JSON:** faster, but pulls Boost.
3. **Hand-written parser:** no dependency, but a maintenance and correctness burden unrelated to the project goal.

## Decision (proposed)
Option 1, pinned to a released version, used only inside `hp_io`. Unknown keys and wrong types are turned into `IoError` values; the JSON library's exceptions never leave `hp_io` (NFR-04).

## Consequences
- The core can be built and tested without the dependency.
- A pinned version (or vendored copy) keeps builds reproducible.
