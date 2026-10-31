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
| S4 | `WaterLoad`, `SelfWeight`, `ForceContribution`, pressure distribution | FR-05 to FR-07 |
| S5 | Resultant, base contact, sliding, overturning report, criteria presets, checks, verdict, warnings; `analyze` / `Analyzer` | FR-08 to FR-16 |
| S6 | JSON I/O, schema v1, golden files, CLI | FR-17 to FR-20 |
| S7 | Hardening: traceability script, Clang build, coverage report, performance check | NFR-05, NFR-07, NFR-08 |

## Later (not scheduled)
- v1.x: uplift and tailwater (new `Load` implementations), cohesion in sliding.
- Visualization front end (`hp_viz`) consuming `Result`.
- Retaining walls, channel and basement walls; sediment and ice loads.
- Efficient binary storage next to JSON.
- Hydrodynamic loads (waves, seepage, seismic); 3D.
- Thermal effects (concrete expansion and contraction, temperature gradients, ice thrust from thermal expansion): out of scope for the rigid-block model; to be scoped later. "No driving shear" in the sliding result means no driving shear among the modelled loads, not zero movement.

## Status and where to resume
Last updated 2026-10-31. S1 to S5 are done and committed; 123 tests pass in the default build and under AddressSanitizer + UBSan (`cmake --preset asan-ubsan`, `ctest --test-dir build-asan`).

S4 delivered `ForceContribution`, the `Load` interface, `WaterLoad` (closed-form per face segment, reported as "water (horizontal)" and "water (vertical)"; exactly-zero components are omitted), `SelfWeight`, and `PressureDistribution` samples via `WaterLoad::pressure()` (FR-05, FR-20). Sign convention: x downstream, y up, so water has positive `fx` and weight negative `fy`.

S5 delivered `Resultant`, `Contact`, sliding, the overturning report, `Checks`, `Result`, `Analyzer` and `analyze()`, plus defaulted `operator==` on all value types. Conventions: moments about the toe, counter-clockwise positive (resisting > 0, overturning < 0); a contribution with both components becomes two report terms; x_R and e are empty when V <= 0; the location check also requires base contact; sliding with H <= 0 reports "no driving shear" and passes; bearing is skipped (not failed) when no allowable pressure is given. AC-1 to AC-6 all match the test plan.

**Next: S6** (JSON I/O and CLI): scenario parsing with schema version and unknown-key rejection, result writer with a golden file, `pressure_samples` input (FR-20) wired into `Analyzer`, CLI exit codes. Then S7 (hardening).

## Open items
- ADR-0007 (numeric tolerance and determinism) is still PROPOSED; ask for approval at the next gate.
- Clang is not installed; NFR-05 (GCC and Clang) cannot be verified until it is.
- The USACE criteria come from a third-party scan of EM 1110-2-2200 (1995); re-verify against the official publication before any compliance claim (decision R3).
