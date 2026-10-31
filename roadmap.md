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

## Status and where to resume
Last updated 2026-10-31. S1 to S4 are done and committed; 83 tests pass in the default build and under AddressSanitizer + UBSan (`cmake --preset asan-ubsan`, `ctest --test-dir build-asan`).

S4 delivered `ForceContribution`, the `Load` interface, `WaterLoad` (closed-form per face segment, reported as "water (horizontal)" and "water (vertical)"; exactly-zero components are omitted), `SelfWeight`, and `PressureDistribution` samples via `WaterLoad::pressure()` (FR-05, FR-20). Tests L-01 to L-04 cover AC-1, AC-3, AC-4 and the extension seam. Sign convention: x downstream, y up, so water has positive `fx` and weight negative `fy`.

**Next: S5**: resultant, contact, sliding, overturning report, checks, verdict, warnings and `analyze`. Then S6 (JSON I/O, CLI) and S7 (hardening).

## Open items
- ADR-0007 (numeric tolerance and determinism) is still PROPOSED; ask for approval at the next gate.
- Clang is not installed; NFR-05 (GCC and Clang) cannot be verified until it is.
- The USACE criteria come from a third-party scan of EM 1110-2-2200 (1995); re-verify against the official publication before any compliance claim (decision R3).
- Possible later additions to units: `operator==` / ordering, needed for `Result` equality (test A-02) in S5.
