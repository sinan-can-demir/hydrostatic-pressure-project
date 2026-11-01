# Hydrostatic Pressure Simulation

C++ engine core for hydrostatic loading on a 2D gravity dam cross-section (stability checks per USACE EM 1110-2-2200). A library plus a JSON-driven CLI; visualization comes later as a separate target.

Status: design complete, implementation in progress. See `roadmap.md`.

## How we work
- **Phase-gated process.** Vision, requirements, use cases, domain model, architecture and test plan were each written, reviewed and approved before code. The documents are in `docs/`:

  | File | Content |
  |------|---------|
  | `docs/01-vision-stakeholders.md` | scope, non-goals, stakeholders, decisions D1 to D5 |
  | `docs/02-requirements.md` | FR/NFR with acceptance criteria, reference criteria, reference case AC-1 |
  | `docs/03-use-cases.md` | actors, use cases UC-01 to UC-09 |
  | `docs/04-domain-model.md` | concepts, invariants, conventions |
  | `docs/05-architecture.md` | layers, API, JSON schema |
  | `docs/06-test-plan.md` | test strategy, reference cases AC-1 to AC-6, traceability |
  | `docs/adr/` | architecture decision records |

- **Test-first vertical slices.** Implementation follows the slices in `roadmap.md`. In each slice the tests are written first and seen to fail, then the code is written. A slice closes only when the exit criteria in `docs/06-test-plan.md` section 9 hold.
- **Requirement traceability.** Tests carry requirement IDs as Catch2 tags (for example `[FR-06]`).
- **Decisions are recorded.** Each gate's decisions are written at the end of its document; design trade-offs live in `docs/adr/`.

## Build and test

    cmake -S . -B build && cmake --build build
    ctest --test-dir build --output-on-failure

## Code style and static analysis

`.clang-format` (derived from the existing code) and `.clang-tidy` are committed (NFR-11). Both are advisory: CI does not enforce them yet.

    clang-format -i $(git ls-files '*.cpp' '*.hpp')               # format
    cmake -S . -B build && run-clang-tidy -p build 'computation/.*\.cpp|io/.*\.cpp|apps/.*\.cpp'

CI (`.github/workflows/ci.yml`, design in `docs/08-ci-pipeline.md`) builds with GCC 14 and Clang 19, runs the sanitizers, the core without the JSON layer and the traceability script on every push to `main` and every pull request.
