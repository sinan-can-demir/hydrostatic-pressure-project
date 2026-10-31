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
Last updated 2026-10-31. S1 to S3 are done and committed; 69 tests pass in the default build and under AddressSanitizer + UBSan (`cmake --preset asan-ubsan`, `ctest --test-dir build-asan`).

**Next: S4.** Written test-first, against the reference cases in `docs/06-test-plan.md` section 3:
1. `ForceContribution` (components in N/m, application point, source name).
2. `Load` interface (ADR-0004) and `WaterLoad`: pressure p = rho g (Hw - y) on the upstream face, clipped at the water level, integrated in closed form per face segment (no quadrature). Tests L-01, L-02: AC-1 (Fx = 490 500 at 3.3333 m), AC-3 (Fy = 98 100 downward at x = 0.6667 m from the heel), AC-4 (Hw = 6: Fx = 176 580 at 2.0 m).
3. `SelfWeight`: W = gamma * area at the centroid (L-03).
4. `PressureDistribution` samples (FR-05, FR-20).
5. Stub `Load` test proving the extension seam (L-04).

Then S5 (resultant, contact, sliding, overturning report, checks, verdict, warnings, `analyze`), S6 (JSON I/O, CLI), S7 (hardening).

## Open items
- ADR-0007 (numeric tolerance and determinism) is still PROPOSED; ask for approval at the next gate.
- Clang is not installed; NFR-05 (GCC and Clang) cannot be verified until it is.
- The USACE criteria come from a third-party scan of EM 1110-2-2200 (1995); re-verify against the official publication before any compliance claim (decision R3).
- Possible later additions to units: `operator==` / ordering, needed for `Result` equality (test A-02) in S5.
