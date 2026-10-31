# 01 Vision and Stakeholders

Status: APPROVED with owner changes (Phase 1 gate).

## 1. Problem statement
Preliminary design and teaching of water-retaining structures requires repeatable hydrostatic load calculations (pressure distribution, resultant force, centre of pressure) and stability checks (overturning, sliding, base pressure). These are commonly done by hand or in ad-hoc spreadsheets, which are error-prone (unit mix-ups, sign errors) and hard to verify or reuse.

## 2. Vision
A real, engineering-grade tool: a C++ calculation core for hydrostatic loading on a 2D gravity dam cross-section, validated against analytic solutions, with strong unit safety. It is usable as a library and through a JSON-driven CLI, and later by a separate visualization front end. The v1 model is deliberately the most fundamental case so every step can be understood and hand-verified.

## 3. Scope (v1)
- Static hydrostatic analysis of a 2D cross-section, per unit width.
- Structure: concrete gravity dam only (simple polygon section, e.g. rectangular and trapezoidal/triangular profiles), resting on a rigid horizontal base.
- Loads: upstream hydrostatic water pressure and dam self-weight. Fluid density is a parameter.
- Outputs: pressure distribution on the upstream face, resultant water force (magnitude, line of action), self-weight and its centroid, resultant on the base.
- Stability checks: overturning about the toe, sliding (friction), and base pressure / resultant location (middle-third check).
- Safety-factor thresholds user-configurable, with a preset based on USACE EM 1110-2-2200.
- Interfaces: C++ library, CLI with JSON scenario in and JSON results out.

## 4. Non-goals (v1)
- Uplift, tailwater, sediment and ice loads (deferred, owner unsure; see D3). Without uplift, v1 results are unconservative for real dams and the tool must say so in its output.
- Hydrodynamic effects: waves, seepage, seismic, flow. Extension seams are designed but not implemented.
- Retaining walls, channel walls, building and basement walls (planned for later versions, reusing the same solver).
- Arch and embankment dams, gates, 3D geometry, structural stress or FEM analysis.
- Certified code compliance: results support preliminary design and education, not a licensed engineer's verification.
- Visualization in the first milestones; it is a later, separate target.

## 5. Stakeholder register
| ID | Stakeholder | Role | Needs | Concerns | Influence |
|----|-------------|------|-------|----------|-----------|
| S1 | Civil engineering student | Primary user (CLI now, visualization later) | Change parameters, see pressure diagrams and results; understand intermediate steps | Opaque results; hard setup | Medium |
| S2 | Practising engineer | Primary user | Trustworthy numbers, explicit units, reproducible runs, exportable results | Silent errors, unverified accuracy, unit confusion, unconservative omissions | High |
| S3 | Developer / integrator | Library consumer | Clean stable C++ API, no GUI dependency, structured errors, easy build | API churn, hidden dependencies, exceptions across boundaries | High |
| S4 | Project owner / maintainer (Sinan Demir) | Builds and maintains | Maintainable architecture, traceable requirements, testability, understandable physics | Scope creep, untested code | Decisive |
| S5 | Instructor (secondary, not a v1 actor) | Future consumer | Shareable scenarios, reference solutions | Deferred; scenario format should not preclude this | Low |

## 6. Decisions
| # | Decision | Rationale |
|---|----------|-----------|
| D1 | Real, engineering-grade tool, also usable for teaching | Owner request; serves S1, S2, S3 |
| D2 | v1 structure: gravity dam only, most fundamental case | Owner must be able to understand and hand-verify everything |
| D3 | Uplift, tailwater, sediment, ice deferred; v1 output carries an explicit "uplift not modelled" warning | Owner unsure; keeps v1 small. Recommended first v1.x addition: uplift, since it governs real gravity-dam stability |
| D4 | Reference standard: USACE EM 1110-2-2200 (Gravity Dam Design); FERC Engineering Guidelines ch. 3 and USBR as cross-checks. Thresholds remain configurable | Most widely cited gravity-dam criteria. Exact criteria values to be read from the source and cited in Phase 2 requirements, not taken from memory |
| D5 | Library plus JSON CLI first; visualization separate, later | Keeps the engine free of rendering dependencies |

## 7. Glossary
- **Hydrostatic pressure**: pressure from a fluid at rest, p = rho * g * h.
- **Resultant force**: single force equivalent to a pressure distribution.
- **Centre of pressure**: point where the resultant acts.
- **Per unit width**: 2D analysis treating the section as 1 m deep.
- **Heel / toe**: upstream / downstream bottom corners of the dam base.
- **Overturning**: failure by rotation about the toe; checked via moment balance.
- **Sliding**: failure by horizontal translation along the base; checked with a friction coefficient.
- **Middle third**: the base resultant should lie within the middle third of the base so no tension develops.
- **Factor of safety (FS)**: ratio of resisting to driving action.
- **Uplift**: upward water pressure under the base (deferred).

## 8. Open questions
- None blocking. Revisit D3 (uplift) at the v1 milestone review.
