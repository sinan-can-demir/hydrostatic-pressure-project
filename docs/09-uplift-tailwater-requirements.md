# 09 Requirements increment: tailwater and uplift (v1.1)

Status: SCOPED AND DEFERRED (owner decision, 2026-11-01). This is a scoped future increment, not scheduled. Section 9 decisions are not yet taken; the recommendations stand as proposals until the increment is picked up and gated.
Inputs: `01-vision-stakeholders.md` (D3, D4), `02-requirements.md` (FR-15, FR-21, CON-04, CON-05, R3), `04-domain-model.md`, ADR-0004.
Source: USACE EM 1110-2-2200 (30 Jun 1995), paragraphs 3-3.c(3) (tailwater), 3-3.d(1) (uplift along the base) and Figures 3-1 to 3-5. The text and the figure formulas were read directly from the document (same third-party copy as decision R3; the same re-verification caveat applies).
Numbering continues `02-requirements.md`: new requirements start at FR-22 and NFR-12.

## 1. Plain-language primer (for the non-civil-engineer reader)
- **Tailwater** is the water on the downstream side of the dam. It pushes back on the downstream face, which helps the dam a little, and it sets the water pressure at the toe.
- **Uplift** is water pressure acting upward on the underside of the dam, from water seeping through cracks and pores between the dam and its rock foundation. It acts like a buoyancy force: it reduces the effective weight (less friction, so less sliding resistance) and it adds an overturning moment. For real gravity dams it is one of the dominant loads, and ignoring it is unconservative. That is why v1 prints the "uplift not modelled" warning.
- **How the standard models it (no drains):** the pressure under the base falls along a straight line from full reservoir pressure at the heel to tailwater pressure at the toe.
- **Drains:** many dams have a line of drain holes under the base. They bleed off pressure, so the diagram becomes two straight segments: heel, then the drain line, then the toe. The **drain effectiveness** E (0 to 1) says how much of the pressure drop the drains achieve. The standard says to assume 25 to 50 percent, and up to 67 percent only with approval and supporting data. This is a judgement the engineer makes; the tool must not choose it silently.
- **Cracked base:** if part of the base is not pressed against the foundation (the resultant is outside the middle part of the base), water can enter that gap and push with full reservoir pressure. That makes the load worse, which moves the resultant, which changes the gap. It needs iteration. This is a separate, harder feature (section 3, D-U1).

## 2. Source rules the engine will implement
All heads are measured as heights of water above the base plane (y = 0). H1 = reservoir depth (Hw), H2 = tailwater depth, L = base length B, X = distance from the heel to the drain line, H4 = elevation of the drain line (gallery floor) above the base, E = drain effectiveness, K = 1 - E.

| Rule | Source | Engine behaviour |
|------|--------|------------------|
| Tailwater pressure on the downstream face uses the full tailwater depth (nonoverflow sections) | 3-3.c(3)(a) | Hydrostatic load on the wetted downstream face, same closed-form method as the upstream water load |
| Uplift acts over 100 percent of the base | 3-3.d(1)(a) | Uplift is integrated over the whole base length B |
| Uplift at the toe uses full tailwater depth, always | 3-3.c(3)(d) | Toe ordinate = rho g H2 |
| No drains: straight line from headwater at the heel to tailwater at the toe | 3-3.d(1)(b), Fig. 3-1 | Trapezoid (triangle when H2 = 0) |
| With drains: straight from heel to the drain line to the toe; head at the drain line H3 = K (H1 - H') (L - X) / L + H', where H' = H4 if H4 > H2, else H2 | 3-3.d(1)(c), Fig. 3-2 | Two-segment (bilinear) diagram. H' implements the rule that a gallery above tailwater is treated as if tailwater were at the gallery elevation |
| Drain effectiveness is assumed between 25 and 50 percent; up to 67 percent only with approval and data | 3-3.d(1)(c) | Input is explicit, no default; outside 25-50 percent produces a warning (section 5, FR-25) |
| Uplift is unchanged by earthquake loads | 3-3.d | Not relevant until seismic loads exist |
| Drains within 0.05 H1 of the upstream face (X <= 0.05 H1): single straight line from H3 = K (H1 - H') + H' at the heel to H2 at the toe | 3-3.d(1)(c), Fig. 3-3 | Special case selected by X <= 0.05 H1 |
| Zero-compression zone carries 100 percent of the adjacent head; if it extends past the drains, drain effectiveness is ignored | 3-3.d(1)(e), Figs. 3-4, 3-5 | Cracked-base iteration (D-U1, FR-29) |
| Uplift inside the dam body: 50 percent of headwater at the upstream face, linear to 50 percent of tailwater (or zero) at the downstream face | 3-3.d(2) | Out of scope (stays W): only the base plane is analysed in 2D rigid-block mode |

## 3. Scope decisions (recommendations; the owner delegated these, so each has its reason)
| # | Decision | Reason |
|---|----------|--------|
| D-U1 | **Two tiers.** Tier A (this increment, Must): tailwater load, base uplift without and with drains, **assuming the full base is in compression**. Tier B (later slice, Should): cracked-base iteration per 3-3.d(1)(e). In tier A, if the computed contact length is less than B (base not fully in compression), the result carries a warning that the uplift is unconservative because the standard requires full uplift on the uncompressed zone. | Tier A is a pure, closed-form `Load`, hand-verifiable and consistent with ADR-0004. Tier B makes uplift depend on the result, which breaks the "loads are pure functions of the scenario" rule and needs its own ADR and convergence tests. The warning keeps the tool honest in between; it fires exactly in the cases where tier A is not valid. |
| D-U2 | Drain effectiveness has **no default**: if drains are given, E is required. Absence of a `drains` block means no drains (E = 0). | A hidden default would be a silent engineering judgement (stakeholder S2 fear: silent, unconservative omissions). |
| D-U3 | Tailwater applies to **nonoverflow sections only** (full depth, no retrogression reduction). Overflow sections stay outside scope (CON-04: no overtopping). | Retrogression (down to 60 percent of depth) applies only to spillway flow. |
| D-U4 | **Cohesion stays out** of this increment (remains CON-05 / W). It needs rock shear-strength data and a contact-length term in the sliding formula, and it is independent of water loads. It becomes its own small increment. | Keeps each gate reviewable and each slice small. |
| D-U5 | Uplift and tailwater are **opt-in per scenario**. A scenario without these inputs behaves exactly as in v1 (same numbers, same warning). | Backward compatibility; the v1 golden file must not change. |
| D-U6 | `gallery_elevation` (H4) is optional, default 0 (drain line at the base). Priority C. | Covers the source rule without forcing the input on common cases. |

## 4. Changes to existing requirements
| Item | Change |
|------|--------|
| FR-15 | Reworded: a result always carries the uplift warning **unless** the scenario models uplift. When uplift is modelled the warning is replaced by a statement of the model used (no drains / drains with E and X) and by the tier A cracked-base warning when it applies. W-01 is updated accordingly. |
| FR-21 | Uplift and tailwater are removed from the Won't list. Cohesion, sediment, ice, hydrodynamic and seismic loads, overtopping, non-horizontal bases and multi-material sections remain. |
| CON-04 | Unchanged (hydrostatic loading, no overtopping). Clarified: tailwater is also hydrostatic. |
| CON-05 | Unchanged (cohesion zero). |
| Domain invariant 3 | The downstream face (crest to toe chain) must not **increase** in elevation going toward the toe, mirroring the upstream rule (M2), so tailwater has a well-defined wetted surface. Validator gains one error code. |
| Schema (`07-json-schema.md`) | New optional keys; see FR-31. |

## 5. New requirements
| ID | Pri | Requirement | Acceptance criterion |
|----|-----|-------------|----------------------|
| FR-22 | M | The user may specify a tailwater depth H2 (m above base), 0 <= H2 <= H1. Omitted means no tailwater. H2 > H1, negative or non-finite values are rejected with a distinct error code. | Boundary tests; H2 = 0 equals the no-tailwater result. |
| FR-23 | M | The engine shall compute the hydrostatic load of tailwater on the wetted downstream face, with the same closed-form method, force components and moment as the upstream water load. It is reported as "tailwater (horizontal)" and "tailwater (vertical)" (exactly-zero components omitted). | Vertical face: Fx = 1/2 rho g H2^2 acting upstream at H2/3 above the base. Sloped face: Fy equals the weight of water above the face. AU-2. |
| FR-24 | M | The engine shall compute base uplift as a pressure distribution over the whole base, from the rules in section 2: no drains (heel rho g H1, toe rho g H2) or with drains (adds the drain-line ordinate H3). It shall return the force (N/m) and its moment about the toe as `ForceContribution` values named "uplift", positive upward, acting on the base. | AU-1 (no drains, no tailwater), AU-2 (tailwater), AU-3 (drains). Independent integration oracle (as P-05). |
| FR-25 | M | Drains are specified by effectiveness E in (0, 1] and distance X from the heel in [0, B]; optional gallery elevation H4 >= 0 (D-U6). Out-of-range values are rejected. E outside [0.25, 0.50] adds a warning; E above 0.67 adds a stronger warning naming the standard's limit. The engine never changes the user's E. | Table tests for each boundary; warning present at 0.51, 0.67, 0.68. |
| FR-26 | M | Uplift and tailwater shall flow through the existing pipeline (resultant, base contact, sliding, overturning report, checks) with no change to the formulas downstream: the loads only add contributions (ADR-0004). The sliding check uses the reduced V. | AC-1 unchanged when the new inputs are absent; AU cases match section 6. |
| FR-27 | S | The overturning report lists the uplift and tailwater terms with force, arm and moment, tagged resisting or overturning (R4, NFR-09). | AU-1 to AU-3 terms listed. |
| FR-28 | M | If uplift is modelled and the computed contact length is less than B, the result carries a warning that the uplift assumes full contact and is unconservative (D-U1). If V <= 0 after uplift (dam floats), the existing "no valid contact" path applies and the result is unstable. | Warning present in the cracked case; absent when the base is fully in compression. A scenario with U > W reports unstable. |
| FR-29 | S | Tier B (later slice): cracked-base uplift by iteration (Figs. 3-4, 3-5): full head on the zero-compression length T, drains ignored when T > X, repeated until T converges; non-convergence is reported as an error code, never a silent result. | Converges on AU-4 (to be hand-computed at the test-plan gate); determinism preserved (NFR-03). |
| FR-30 | S | The result includes the uplift pressure distribution sampled along the base (heel to toe, including the drain-line point), usable for plotting (as FR-05 for the face). | Samples match the diagram at heel, drain line, toe. |
| FR-31 | M | JSON: optional `tailwater_depth_m` and `drains` object (`effectiveness`, `distance_from_heel_m`, optional `gallery_elevation_m`) in the scenario; results include the new contributions, uplift samples and warnings. Schema version handling per decision D-U7 below. | Round-trip and golden-file tests; unknown keys still rejected. |
| NFR-12 | M | v1 behaviour is preserved: for any scenario without the new inputs, the result (numbers and warnings) is identical to the v1 golden file `tests/data/ac1_result.json`. | Existing golden test passes unchanged. |

Open design decision D-U7 (needs the architecture step, not the owner): whether the added optional keys bump `schema_version` to 2 (accepting 1 and 2) or stay under 1. Recommendation: bump to 2, accept both, because older parsers reject unknown keys and a version number is the documented compatibility signal (FR-19).

## 6. Reference acceptance cases (hand calculations)
Common data: rectangle B = 5 m, H = 10 m, vertical faces, Hw = H1 = 10 m, rho = 1000, g = 9.81, gamma = 23 500, phi = 35 degrees (tan = 0.70021). Weight W = 1 175 000 N/m acting at 2.5 m from the toe. Upstream water Fx = 490 500 N/m, overturning moment -1 635 000 N*m/m. Moments about the toe, CCW positive (resisting > 0). In the table, "head-moment" means head-area times rho g (9810).

**AU-1: uplift, no drains, no tailwater.** Pressure: heel 98 100 Pa, toe 0.
- Uplift U = 1/2 * 98 100 * 5 = 245 250 N/m, centroid 5/3 from the heel = 3.3333 m from the toe, so moment = -817 500.
- V = 1 175 000 - 245 250 = 929 750. M = 2 937 500 - 1 635 000 - 817 500 = 485 000.
- x_R = 0.5216 m (middle third limit 1.6667 m: fails location, as in AC-1 but worse). Contact length 3 x_R = 1.565 m < B, so FR-28 warning fires. Sliding FS = 929 750 * 0.70021 / 490 500 = 1.327.

**AU-2: tailwater H2 = 3 m plus uplift, no drains.**
- Tailwater Fx = -1/2 * 9810 * 9 = -44 145 N/m at 1.0 m above the base: moment +44 145.
- Uplift: heel 98 100, toe 29 430. Rectangle 29 430 * 5 = 147 150 at 2.5 m (moment -367 875) plus triangle 1/2 * 68 670 * 5 = 171 675 at 3.3333 m (moment -572 250). U = 318 825, moment -940 125.
- V = 856 175. H = 446 355. M = 2 937 500 - 1 635 000 + 44 145 - 940 125 = 406 520.
- x_R = 0.4748 m. Sliding FS = 856 175 * 0.70021 / 446 355 = 1.343.

**AU-3: drains, no tailwater.** E = 0.5 (K = 0.5), X = 1 m, H4 = 0, H2 = 0.
- H3 = K (H1 - H') (L - X) / L + H' = 0.5 * 10 * 4/5 = 4 m. Diagram: heel 10 m, drain 4 m, toe 0 m.
- Head-areas: heel to drain (10 + 4)/2 * 1 = 7; drain to toe 4/2 * 4 = 8; total 15, so U = 15 * 9810 = 147 150 N/m.
- Head-moments about the toe: 4 * 1 at 4.5 m = 18; 1/2 * 6 * 1 at 4.6667 m = 14; 8 at 1.3333 m = 10.667; total 42.667, so moment = -418 560.
- V = 1 027 850. M = 2 937 500 - 1 635 000 - 418 560 = 883 940. x_R = 0.8600 m. Sliding FS = 1.467.

These are first hand calculations and must be re-derived independently (script oracle) at the test-plan gate, as AC-1 was. AU-4 (cracked base) is defined when FR-29 is scheduled.

## 7. Impact on existing design
- **ADR-0004 holds.** Add `TailwaterLoad` and `UpliftLoad` (both pure, tier A). Resultant, contact, sliding, overturning report and checks are unchanged (FR-26). A new ADR is needed only for tier B, because uplift would then depend on the contact result.
- **`WaterLoad` generalisation:** it currently handles the upstream face; tailwater reuses the same segment integration for the downstream face. Prefer extracting the wetted-face integration over copying it.
- **Validator:** new rules for H2, drains, and the downstream-face invariant.
- **Test plan (`06-test-plan.md`):** new section with analytic cases AU-1 to AU-3, boundary tests, a property test with an independent integration oracle for the uplift diagram, mutation check of the new loads, and the NFR-12 regression test. Traceability script covers FR-22 to FR-28, FR-31, NFR-12.
- **CI:** no change.
- **Docs:** `04` (loads, invariants), `05` (component list), `07` (schema), `README`, roadmap.

## 8. Proposed slices (after the gate)
| Slice | Content | Requirements |
|-------|---------|--------------|
| U1 | Test plan section, data model (`tailwater_depth`, `Drains`), validation, downstream-face invariant | FR-22, FR-25 (validation) |
| U2 | `TailwaterLoad` | FR-23 |
| U3 | `UpliftLoad` (no drains, then drains), pipeline integration, warnings | FR-24, FR-26, FR-27, FR-28, FR-15, NFR-12 |
| U4 | JSON, schema doc, golden files, uplift samples | FR-30, FR-31 |
| U5 (later) | Cracked-base iteration (tier B), own ADR | FR-29 |

Each slice follows the existing process: tests first, exit criteria from `06-test-plan.md` section 9, PR into `main` with CI green.

## 9. Gate decisions (pending: to be taken when the increment is scheduled)
Please confirm or change (a "your call" is accepted and means the recommendations stand):
1. D-U1 to D-U6 as written in section 3.
2. D-U7 schema version bump (recommended: yes).
3. Warning thresholds on E in FR-25 (25 to 50 percent normal, above 67 percent strong warning).

## 10. Risks and open items
- The source is a third-party scan (R3). The formulas for H3 and the E limits were read from the document; confirm against the official publication before any compliance claim.
- "Drain effectiveness" is defined only by the formula in Fig. 3-2 (K = 1 - E applied to the pressure drop from heel to drain line); the engine uses exactly that definition and reports E and H3 in the result.
- Figure 3-3 (drains within 0.05 H1 of the upstream face, X <= 0.05 H1) is a **distinct rule, not a limit of the bilinear one**: the heel ordinate itself drops to H3 = K (H1 - H') + H' and the diagram is a single straight line from H3 at the heel to H2 at the toe. It is a requirement (FR-24 covers it) with its own test; reading of the figure text to be re-checked at the test-plan gate.
- Tier A is unconservative when the base is not fully in compression; mitigated by FR-28 until tier B.
- Uplift inside the dam body and at construction joints (3-3.d(2)) is out of scope.
