# 08 Continuous integration pipeline

Status: APPROVED (gate passed 2026-10-31, all five decisions in section 8 as recommended). Implementation is not started; `.github/` does not exist yet.
Requirements: NFR-05 (GCC and Clang build matrix, warnings as errors), NFR-07 (traceability), NFR-08 (performance, advisory), NFR-03 (determinism), test plan section 8 (verification).
Sources: `CMakePresets.json`, `scripts/traceability.py`, `scripts/coverage.py`, `tests/CMakeLists.txt`.

## 1. Purpose
Today every quality claim in `roadmap.md` (163 tests on GCC, Clang and the sanitizers; traceability; coverage) is checked by hand on one machine. CI repeats those checks on a clean machine for every push, so a regression, a missing dependency or a file that exists only locally is caught before it is merged. It adds no new checks of its own: each job runs a command that already exists.

## 2. Scope
In scope: GitHub Actions workflow on Linux x86-64, triggered by push to `main` and by pull requests; the jobs in section 3; dependency caching; how failures are reported.
Out of scope: Windows and macOS builds (NFR-05 says Linux), releases and packaging, deployment, formatting and lint gates (NFR-11 is not done), automatic dependency updates.

## 3. Jobs
All jobs configure with the existing presets, so CI and a developer run the same commands.

| Job | Command | What it proves | Blocks merge |
|-----|---------|----------------|--------------|
| `gcc` | `cmake --preset default`, build, `ctest --preset default` | all tests pass, warnings are errors (NFR-05) | yes |
| `clang` | same with preset `clang` | the second compiler (NFR-05) | yes |
| `sanitizers` | preset `asan-ubsan` | no memory errors or undefined behaviour in 163 tests | yes |
| `traceability` | `python3 scripts/traceability.py` | every Must requirement is named in a test (NFR-07) | yes |
| `release-bench` | Release build, `ctest -L advisory` | analysis of 1000 vertices stays near its budget (NFR-08) | no (advisory) |
| `coverage` | preset `coverage`, `scripts/coverage.py` | line coverage report, uploaded as an artifact | no (advisory) |
| `core-without-io` | `-DHP_BUILD_IO=OFF` build and test | the core builds without the JSON library (N-04) | yes |

Notes:
- `traceability` needs only Python, so it runs in seconds and gives the fastest feedback.
- The negative build tests (`build_graph_NFR-06_*`) run inside `gcc` and `clang` as ordinary CTest tests.
- `release-bench` reports the timing in the job log. A slow shared runner must never fail a build, so timing is never a gate (as in the test plan).
- `coverage` publishes the report; it does not fail below 90 percent in v1, matching "advisory, not a merge gate" in `coverage.py`.

## 4. Toolchains
| Item | Choice | Reason |
|------|--------|--------|
| Runner | `ubuntu-24.04` (pinned, not `ubuntu-latest`) | `latest` changes under us and would change results with no commit |
| GCC | 14 (`g++-14` from the Ubuntu repository) | needs `std::format` and `std::expected`, so GCC 13 or newer |
| Clang | 19 or newer from the Ubuntu or apt.llvm.org packages | local builds use Clang 22; Ubuntu 24.04 does not ship it, so the CI minimum is the newest it can install reliably |
| CMake | the runner's preinstalled version (>= 3.21 required by `CMakeLists.txt`) | no extra install step |
| Python | the runner's `python3` | scripts use only the standard library |

The local machine uses GCC 16 and Clang 22, so CI also checks that nothing relies on a very recent compiler. This is a real benefit of CI and also the main risk (section 7).

## 5. Dependencies and caching
Catch2 v3.9.1 and nlohmann/json v3.12.0 are downloaded by `FetchContent` at configure time. Both are pinned by tag, so builds are reproducible but need network access and take a few seconds. Plan:
- Cache the `_deps` download directory per build directory, keyed on the hash of `CMakeLists.txt`, so a normal push does not re-download.
- Use no compiler cache (ccache) in v1. A full build is short; revisit if a job exceeds five minutes.

## 6. Failure policy
- A pull request needs the blocking jobs green before merge. Branch protection is a GitHub setting the repository owner turns on; the workflow cannot enforce it.
- On failure, CTest prints the output of failed tests (`outputOnFailure` is already set in the test presets).
- Jobs run in parallel and are independent, so one failure does not hide another.
- The workflow cancels an older run of the same branch when a newer push arrives, to save runner minutes.

## 7. Risks
| Risk | Effect | Mitigation |
|------|--------|-----------|
| Older compilers than local (GCC 14, Clang 19 vs 16 and 22) reject or warn on code that local builds accept; warnings are errors | first CI run is red | expected; fix the code in the first run, not the workflow; this is the reason for NFR-05 |
| Clang 19 and Catch2's `__COUNTER__` use | the local Clang flag `-Wno-c2y-extensions` may not exist in older Clang | check the first run; guard the flag with a compiler-support check if needed |
| `FetchContent` network failure | spurious red build | the cache; a rerun |
| Release timing on shared runners | noisy benchmark | advisory only |
| The benchmark and sanitizers add minutes | slower feedback | all jobs run in parallel |
| Third-party GitHub Actions can change | supply-chain exposure | use only `actions/checkout`, `actions/cache` and `actions/upload-artifact`, pinned to a commit SHA, with read-only repository permissions |

## 8. Decisions (approved as recommended)
1. **Which jobs block a merge.** Approved: `gcc`, `clang`, `sanitizers`, `traceability`, `core-without-io` block; `release-bench` and `coverage` are advisory.
2. **Compiler versions.** Approved: GCC 14 and Clang 19 or newer, on pinned `ubuntu-24.04`.
3. **Triggers.** Approved: every push to `main` and every pull request; no scheduled runs.
4. **Actions pinned to commit SHAs** (approved) versus version tags such as `@v4` (easier to read, less safe).
5. **First workflow on a branch** (approved): CI is added on a branch, the older-compiler fixes are made there, and it is merged green, rather than pushing a red `main`.

## 9. Exit criteria
- The workflow file is in `.github/workflows/ci.yml` and every job in section 3 passes on a clean run.
- The README and `roadmap.md` mention CI, and NFR-05 in `docs/06-test-plan.md` changes from "verified manually via presets" to "verified by CI".
- A deliberate failure on a throwaway branch (for example removing a requirement ID from the only test that names it) turns the right job red, then is discarded.
