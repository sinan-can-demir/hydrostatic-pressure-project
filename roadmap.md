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
| S1 | Test infrastructure (Catch2, CTest, sanitizer preset); unit types complete (`Area`, `UnitWeight`, `Angle`, `ForcePerWidth`, `MomentPerWidth`, `Pressure * Length`); compile-fail tests | NFR-02 |
| S2 | `Point2`, `Vector2`, `Fluid`, `Concrete`, `Scenario` value types | FR-03 |
| S3 | `DamSection` geometry and `Validator` (all errors at once) | FR-01, FR-02, FR-04, NFR-10 |
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
