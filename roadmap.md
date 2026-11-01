# Roadmap

Process: each design phase is a gate (documents in `docs/`); implementation is test-first in vertical slices, each closed by the exit criteria in `docs/06-test-plan.md` section 9.

## Design (done)
| Phase | Artifact | Status |
|-------|----------|--------|
| 0 Repo hygiene | `.gitignore`, CMake skeleton, README | done |
| 1 Vision and stakeholders | `docs/01-vision-stakeholders.md` | approved |
| 2 Requirements | `docs/02-requirements.md` | approved |
| 3 Use cases | `docs/03-use-cases.md` | approved |
| 4 Domain model and architecture | `docs/04-domain-model.md`, `docs/05-architecture.md`, `docs/adr/` | approved (ADR-0007 pending) |
| 5 Test plan | `docs/06-test-plan.md` | approved |

## Implementation (Phase 6)
| Slice | Content | Main requirements |
|-------|---------|-------------------|
| S1 (done) | Test infrastructure (Catch2, CTest, sanitizer preset); unit types complete (`Area`, `UnitWeight`, `Angle`, `ForcePerWidth`, `MomentPerWidth`, `Pressure * Length`); compile-fail tests | NFR-02 |
| S2 (done) | `Point2`, `Vector2`, `Fluid`, `Concrete`, `Scenario` and its parts as plain data, `CriteriaSet` with the USACE Table 4-1 preset. `DamSection` is only the vertex list here; geometry and validation arrive in S3 | FR-03, FR-13 (data) |
| S3 (done) | `DamSection` geometry and `Validator` (all errors at once) | FR-01, FR-02, FR-04, NFR-10 |
| S4 (done) | `WaterLoad`, `SelfWeight`, `ForceContribution`, pressure distribution | FR-05 to FR-07 |
| S5 (done) | Resultant, base contact, sliding, overturning report, criteria presets, checks, verdict, warnings; `analyze` / `Analyzer` | FR-08 to FR-16 |
| S6 (done) | JSON I/O, schema v1, golden files, CLI | FR-17 to FR-20 |
| S7 (done) | Hardening: traceability script, coverage report, performance check, N-04 negative build test, JSON schema document, P-05 property test | FR-19, NFR-06, NFR-07, NFR-08 |

## Later (not scheduled)
- v1.x: uplift and tailwater (new `Load` implementations), cohesion in sliding.
- NFR-11 (Could): committed `.clang-format` / `.clang-tidy` configuration; not done in v1.
- Visualization front end (`hp_viz`) consuming `Result`.
- Retaining walls, channel and basement walls; sediment and ice loads.
- Efficient binary storage next to JSON.
- Hydrodynamic loads (waves, seepage, seismic); 3D.
- Thermal effects (concrete expansion and contraction, temperature gradients, ice thrust from thermal expansion): out of scope for the rigid-block model; to be scoped later. "No driving shear" in the sliding result means no driving shear among the modelled loads, not zero movement.

## Status and where to resume
Last updated 2026-10-31. **All implementation slices S1 to S7 are done; v1 is feature complete.** 163 tests pass with GCC, with Clang 22 (`cmake --preset clang`) and under AddressSanitizer + UBSan (`cmake --preset asan-ubsan`, `ctest --test-dir build-asan`).

S4 delivered `ForceContribution`, the `Load` interface, `WaterLoad` (closed-form per face segment, reported as "water (horizontal)" and "water (vertical)"; exactly-zero components are omitted), `SelfWeight`, and `PressureDistribution` samples via `WaterLoad::pressure()` (FR-05, FR-20). Sign convention: x downstream, y up, so water has positive `fx` and weight negative `fy`.

S5 delivered `Resultant`, `Contact`, sliding, the overturning report, `Checks`, `Result`, `Analyzer` and `analyze()`, plus defaulted `operator==` on all value types. Conventions: moments about the toe, counter-clockwise positive (resisting > 0, overturning < 0); a contribution with both components becomes two report terms; x_R and e are empty when V <= 0; the location check also requires base contact; sliding with H <= 0 reports "no driving shear" and passes; bearing is skipped (not failed) when no allowable pressure is given. AC-1 to AC-6 all match the test plan.

S6 delivered `Scenario::pressure_samples` (validated 2 to 100000), `computation/io` (`hp_io`: `parse_scenario`, `scenario_to_json`, `result_to_json`; nlohmann/json v3.12.0 confined here; `-DHP_BUILD_IO=OFF` builds the core without it), and `apps/cli` (`hp_cli [input.json | -] [-o output.json]`, logic in `cli::run`). Exit codes: 0 analysis ran (a FAIL verdict included), 1 malformed or invalid scenario (all errors on stderr, nothing written), 2 usage or file error. The parser reports every structural problem and rejects unknown keys; criteria rows override the preset field by field; the echoed friction angle is chosen so a re-run reproduces the result exactly. The golden file is `tests/data/ac1_result.json` (`HP_UPDATE_GOLDEN=1` regenerates it; review it against the AC-1 values before committing).

S7 delivered:
- `scripts/traceability.py`: every Must requirement ID appears in at least one test (text match over `tests/`; NFR-05 and NFR-07 are declared as verified by the Clang build and by the script itself).
- `scripts/coverage.py` with `cmake --preset coverage`: line coverage hp_math 100%, hp_core 95.3% (target 90%); hp_io 90.0%. Advisory, not a gate.
- N-04: `io/` moved out of `computation/` (to `io/include/io`, `io/src`) so `hp_core` cannot reach `hp_io` headers; two `build_graph_NFR-06_*` tests prove that including `io/Json.hpp` or `<nlohmann/json.hpp>` from a core target fails to compile.
- N-03: `hp_bench_analyze` (label `advisory`): 1000-vertex section takes 3.0 ms in a Release build (budget 10 ms), 32 ms in the default unoptimised build. The first measurement was 110 ms Release; validation was almost all of it, fixed by bounding-box culling in `math::is_simple` (results unchanged, regression test added).
- P-05 (1500 seeded random sections, independent oracles: Fx = 1/2 rho g Hw^2, wedge area and centroid, shoelace weight, Simpson-integrated contact force and moment) and N-01 (1000 bitwise-identical repeats). A mutation check on `WaterLoad`, `SelfWeight` and the contact formulas showed all five mutants caught after two oracles were added for the water force positions.
- `docs/07-json-schema.md` (FR-19), including the stable error-code names, now covered by tests.

**Next:** nothing is scheduled. Candidates are in "Later" above; the open items need your decision first.

## Open items
- ADR-0007 (numeric tolerance and determinism) is still PROPOSED; ask for approval at the next gate.
- The USACE criteria come from a third-party scan of EM 1110-2-2200 (1995); re-verify against the official publication before any compliance claim (decision R3).
