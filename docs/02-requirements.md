# 02 Requirements

Status: APPROVED with owner decisions (Phase 2 gate), see section 8.
Inputs: `01-vision-stakeholders.md` (scope D1-D5).
Priority scale (MoSCoW): **M** must, **S** should, **C** could, **W** won't (v1).

## 1. Conventions
- **Coordinates:** 2D section, x horizontal pointing downstream (upstream face on the left), y vertical up. Dam base lies on y = 0. Per unit width (1 m out of plane).
- **Heel / toe:** heel is the upstream base corner (min x of base), toe the downstream base corner (max x). Base length B = x_toe - x_heel.
- **Units:** SI only (m, kg, N, Pa, degrees for angles). Forces are per metre of width (N/m), moments per metre (N*m/m).
- **Section polygon:** vertices in counter-clockwise order.
- **Water:** upstream water level Hw is an elevation above the base (y = Hw). Water acts on the part of the upstream face below Hw.

## 2. Reference criteria (verified source)
Source: USACE EM 1110-2-2200 *Gravity Dam Design*, 30 Jun 1995, Table 4-1 "Stability and stress criteria" and Eqs. 4-1 to 4-3 (verified against the official USACE publication on 2026-11-01, see R3).

| Load condition | Resultant location at base | Minimum sliding FS |
|----------------|----------------------------|--------------------|
| Usual | Middle 1/3 | 2.0 |
| Unusual | Middle 1/2 | 1.7 |
| Extreme | Within base | 1.3 |

Sliding FS (Eq. 4-3): FS = (N tan(phi) + c L) / T, where N is the normal resultant, T the applied shear, c cohesion, L the base length in compression.

Notes on the source that affect the design:
- EM 1110-2-2200 checks **overturning through resultant location**, not an overturning-moment factor of safety. A moment ratio (resisting/overturning about the toe) is still the classic textbook measure, so v1 reports it as informational only, not as a code criterion.
- The same table also contains foundation bearing and concrete stress criteria; v1 only reports base pressure and compares against a user-supplied allowable. Table 4-1 allows **1.33 x allowable** bearing pressure in the extreme condition; the engine applies the plain allowable in every condition, which is stricter for extreme loads (known difference, open item).

## 3. Assumptions and constraints
| ID | Statement |
|----|-----------|
| CON-01 | Implementation language C++23 (ADR-0001); core has no third-party dependency other than a JSON library confined to the I/O layer and a test framework. |
| CON-02 | Rigid, horizontal foundation; base pressure varies linearly (with no tension) across the contact length. |
| CON-03 | Constant water density and gravity; concrete has a single uniform unit weight. |
| CON-04 | Hydrostatic loading only; no overtopping (water level must not exceed the crest). |
| CON-05 | Cohesion is zero in v1 (c = 0); Eq. 4-3 reduces to FS = N tan(phi) / T. |
| CON-06 | Load condition (usual / unusual / extreme) is selected by the user and only picks the criteria thresholds; there is one water level per run. |

## 4. Functional requirements
| ID | Pri | Requirement | Acceptance criterion |
|----|-----|-------------|----------------------|
| FR-01 | M | The user shall define the dam as a polygon of at least 3 vertices (m). | A rectangle, a right triangle and a trapezoid are accepted. |
| FR-02 | M | The engine shall validate the section: simple (non-self-intersecting), positive area, one horizontal base edge on y = 0, no vertex below y = 0, CCW order. Clockwise input is rejected, never reordered (decision R2). The upstream face (heel to crest chain) must not decrease in elevation (decision M2, see `04-domain-model.md`). | Each invalid case, including clockwise order, returns a distinct error code and a message naming the problem; a face that dips (elevation decreasing from heel to crest) is rejected. |
| FR-03 | M | The user shall specify fluid density (kg/m3), gravitational acceleration (m/s2) and concrete unit weight (N/m3). Defaults: 1000, 9.81, 23 500. | Defaults are applied when omitted; non-positive values are rejected. |
| FR-04 | M | The user shall specify the water level Hw (m above base). Hw in [0, crest height]. | Hw above crest is rejected (CON-04); Hw = 0 yields zero water load. |
| FR-05 | M | The engine shall compute the hydrostatic pressure p = rho g (Hw - y) on the wetted upstream face, normal to the face, and return it sampled at the face vertices plus water-surface point. | For a vertical face, p at the base equals rho g Hw within NFR-01. |
| FR-06 | M | The engine shall compute the resultant water force as horizontal and vertical components (Fx, Fy, N/m) and its line of action (point on the face or moment about the toe). | Vertical face: Fx = 1/2 rho g Hw^2 acting at Hw/3 above base; inclined face: Fy equals the weight of water above the face. |
| FR-07 | M | The engine shall compute the dam self-weight W = gamma * area and its centroid. | Rectangle B x H: W = gamma B H at B/2 from heel. |
| FR-08 | M | The engine shall combine loads into the base resultant: sum of vertical forces V, horizontal forces H, moment about the toe, resultant location x_R measured from the toe, and eccentricity e from the base centre. | Matches reference case AC-1. |
| FR-09 | M | The engine shall compute the base contact length and the base pressures at heel and toe: linear distribution if x_R is in the middle third, otherwise triangular over contact length a = 3 x_R (measured from the toe), with zero pressure at the heel end of the contact. If the resultant falls outside the base (x_R <= 0 or x_R >= B), the section is reported as unstable (no valid contact). | AC-1 values; resultant outside the base reports an unstable result. |
| FR-10 | M | The engine shall compute the sliding factor of safety FS = V tan(phi) / H (CON-05) from a user-supplied friction angle phi (degrees). | AC-1 value; H = 0 returns "no driving shear" rather than dividing by zero. |
| FR-11 | S | The engine shall report the informational overturning ratio about the toe (resisting moments / overturning moments) together with every contributing term: each force, its lever arm and its moment, tagged resisting or overturning (decision R4). Not a pass/fail criterion. | AC-1 values including each term; zero overturning moment is reported as unbounded. |
| FR-12 | M | The engine shall evaluate pass/fail for resultant location and sliding FS against thresholds for the chosen load condition. | Pass/fail flips exactly at the threshold in boundary tests. |
| FR-13 | M | Thresholds shall be configurable, with a built-in preset equal to Table 4-1 (Section 2). | Preset values match Section 2; user override replaces them. |
| FR-14 | S | The user may supply an allowable foundation bearing pressure; the engine reports whether the maximum base pressure is within it. | Pass/fail check; omitted input skips the check. |
| FR-15 | M | Every result shall carry the warning "uplift not modelled; results are unconservative for real dams" (D3). From v1.1, a scenario that requests uplift carries a model statement ("uplift modelled ... assuming the whole base is in compression") instead; all other scenarios keep the original warning unchanged. | Warning present in JSON, CLI text and library result (W-01); replaced when uplift is modelled (W-01a). |
| FR-16 | M | The library API shall expose the whole analysis as a pure function from a Scenario value to a Result value, with no I/O or global state. | Callable from a test without files or a CLI. |
| FR-17 | M | The CLI shall read a JSON scenario (file or stdin) and write JSON results (file or stdout). | Round-trip of AC-1 scenario produces expected JSON. |
| FR-18 | M | The CLI shall exit with 0 on success, a non-zero code on invalid input or analysis error, and print the error to stderr. | Exit codes are covered by CLI tests. |
| FR-19 | S | The JSON schema shall be documented and versioned (a `schema_version` field). | Unknown versions are rejected with a clear error. |
| FR-20 | C | The pressure sampling resolution along the face is configurable. | Optional `pressure_samples` yields that many evenly spaced points. |
| FR-21 | W | Uplift, tailwater, sediment, ice, hydrodynamic and seismic loads, cohesion, overtopping, non-horizontal bases, multi-material sections. | Listed as non-goals; the API leaves extension points (Phase 4). |
| FR-22 | M | (v1.1, docs/09) The user may specify a tailwater depth H2 (m above base), 0 <= H2 <= Hw; omitted means none. Invalid values (negative, non-finite, above Hw) and a downstream face that drops going up from the toe are rejected, each with a distinct error code. | Boundary tests; H2 = 0 equals the no-tailwater result (checked in U2, TW-03). |
| FR-23 | M | (v1.1, docs/09) The engine shall compute the hydrostatic load of tailwater on the wetted downstream face with the same closed-form method as the upstream water load, reported as "tailwater (horizontal)" and "tailwater (vertical)" (exactly-zero components omitted). | Vertical face: Fx = 1/2 rho g H2^2 acting towards the upstream side at H2/3 above the base; sloping face: Fy equals the weight of water above the face; AU-2 without uplift. |
| FR-24 | M | (v1.1, docs/09) When the scenario has an `uplift` request, the engine shall compute base uplift from EM 1110-2-2200 para. 3-3.d: heads H1 at the heel and H2 (tailwater, or 0) at the toe; without drains a straight line; with drains the drain-line head H3 = K (H1 - H') (L - X) / L + H' (K = 1 - E; H' = gallery elevation if above the tailwater, else the tailwater), and for drains within 0.05 H1 of the heel (Fig. 3-3) one straight line from H3 = K (H1 - H') + H' at the heel to H2 at the toe. It returns one upward force "uplift" at the centroid of the diagram. | AU-1, AU-2, AU-3, AU-5 values; gallery above and below the tailwater; threshold X = 0.05 H1 selects the two rules; each load mutation is caught. |
| FR-25 | M | Drain effectiveness has no default: drains require E. The engine never changes E; E outside [0.25, 0.50] adds a warning, E above 0.67 a stronger warning naming the standard's limit (validation of ranges: U1). | Warnings at 0.24, 0.51, 0.67 (range) and 0.68 (limit); none at 0.25, 0.30, 0.50. |
| FR-26 | M | Uplift and tailwater flow through the existing pipeline (resultant, contact, sliding, overturning report, checks) with no change to the downstream formulas; the sliding check uses the reduced V. | AU-1 to AU-3, AU-5 through `analyze`: V, H, M, x_R, FS. |
| FR-27 | S | The overturning report lists the uplift and tailwater terms with force, arm, moment and role. | Uplift reported as an overturning vertical term; tailwater as resisting. |
| FR-28 | M | If uplift is modelled and the base is not fully in compression, the result carries a warning that the uplift assumes full contact and is unconservative (D-U1). If V <= 0 after uplift, the existing no-contact path applies and the verdict is fail. | Warning present for partial contact and for U > W; absent for full contact. |
| FR-30 | S | (v1.1, docs/09) The result includes the uplift pressure diagram as break points from heel to toe (heel, drain line where it applies, toe), usable for plotting (as FR-05 for the face). | Samples match the diagram at heel, drain line, toe (AU-3: 98 100, 39 240, 0 Pa). |
| FR-31 | M | (v1.1, docs/09) JSON schema version 2: optional `tailwater_depth_m` and `uplift` (with optional `drains`); results carry the new contributions, `uplift_pressure` and warnings. Versions 1 and 2 are accepted; version 1 documents (and scenarios without the new inputs) behave and are written exactly as before. | Round-trip, golden files AC-1 (unchanged) and AU-3, version 3 rejected, new keys rejected in version 1. |

## 5. Non-functional requirements
| ID | Pri | Category | Requirement | Verification |
|----|-----|----------|-------------|--------------|
| NFR-01 | M | Accuracy | Results match analytic or hand-calculated references with relative error <= 1e-9 (absolute 1e-9 for zero references). | Reference-case tests (AC-1, others in Phase 5). |
| NFR-02 | M | Unit safety | Public API uses strong unit types; nonsensical operations (e.g. Pressure * Length) do not compile. | Compile-fail tests. |
| NFR-03 | M | Determinism | Identical input yields identical output on the same platform and compiler; no randomness, no time dependence, no uninitialised reads. | Repeat-run and sanitizer tests. |
| NFR-04 | M | Error handling | No exceptions cross the public API; failures are returned as typed results with code and message. | API tests for each error path. |
| NFR-05 | M | Portability | Builds with GCC and Clang (C++23) on Linux via CMake >= 3.20; warnings as errors. | CI or local build matrix. |
| NFR-06 | M | Architecture | The engine has no dependency on visualization or the CLI; dependencies point inward (units -> math -> model -> API). | CMake target graph check. |
| NFR-07 | M | Testability | Every M-priority FR has at least one automated test; the matrix in Section 6 is kept current. | Traceability matrix review. |
| NFR-08 | S | Performance | A section with <= 1000 vertices is analysed in < 10 ms on a typical desktop. | Benchmark test. |
| NFR-09 | S | Transparency | Each output is documented with its formula, and results include intermediate values (forces, arms, moments) so a user can hand-verify them. | Doc review; AC-1 intermediate values present. |
| NFR-10 | S | Robustness | The engine rejects NaN, infinite and absurd inputs (e.g. coordinates beyond 1e6 m) instead of propagating them. | Fuzz or table tests on invalid input. |
| NFR-11 | C | Maintainability | Public headers carry short comments for units and conventions; clang-tidy/clang-format configuration is committed. | Config present. |
| NFR-12 | M | Compatibility | (v1.1) A scenario without the new inputs gives exactly the v1 result: same numbers, same contributions, same warning. | Existing golden file `tests/data/ac1_result.json` unchanged; N-05. |

## 6. Reference acceptance case AC-1 (hand calculation)
Rectangular dam, B = 5 m, H = 10 m, vertical upstream face, Hw = 10 m, rho = 1000 kg/m3, g = 9.81 m/s2, gamma = 23 500 N/m3, phi = 35 degrees.

| Quantity | Value | Derivation |
|----------|-------|------------|
| Water force Fx | 490 500 N/m | 1/2 * 1000 * 9.81 * 10^2 |
| Water force arm above base | 3.3333 m | Hw / 3 |
| Self-weight W | 1 175 000 N/m | 23 500 * 5 * 10 |
| W arm from toe | 2.5 m | B / 2 |
| Resisting moment about toe | 2 937 500 N*m/m | 1 175 000 * 2.5 |
| Overturning moment about toe | 1 635 000 N*m/m | 490 500 * 10/3 |
| Overturning ratio | 1.7966 | 2 937 500 / 1 635 000 |
| Resultant location x_R from toe | 1.1085 m | (2 937 500 - 1 635 000) / 1 175 000 |
| Middle third limit | 1.6667 m from toe | B / 3 |
| Eccentricity e from centre | 1.3915 m (> B/6 = 0.8333) | 2.5 - 1.1085 |
| Contact length a | 3.3255 m | 3 * 1.1085 |
| Toe pressure | about 0.707 MPa | 2 V / a |
| Sliding FS | about 1.677 | 1 175 000 * tan(35 deg) / 490 500 |

Expected checks: the usual case fails on resultant location (1.1085 m < 1.6667 m) and on sliding (1.677 < 2.0); the extreme case passes the location check (inside the base) and sliding (1.677 >= 1.3). This also shows why the stated uplift warning matters: adding uplift would make it worse.

## 7. Traceability matrix (skeleton; tests filled in Phase 5)
| Req | Stakeholder | Source / rationale | Test (planned) |
|-----|-------------|--------------------|----------------|
| FR-01, FR-02 | S2, S3 | Reject bad geometry before computing | geometry_validation tests |
| FR-03, FR-04 | S1, S2 | Basic inputs | input_validation tests |
| FR-05 to FR-07 | S1, S2 | Textbook hydrostatics | analytic tests (rectangle, triangle, trapezoid) |
| FR-08 to FR-12 | S2 | EM 1110-2-2200 Table 4-1, Eqs. 4-1, 4-3 | AC-1 and boundary tests |
| FR-13, FR-14 | S2 | Configurable standards | threshold tests |
| FR-15 | M | Every result shall carry the warning "uplift not modelled; results are unconservative for real dams" (D3). From v1.1, a scenario that requests uplift carries a model statement ("uplift modelled ... assuming the whole base is in compression") instead; all other scenarios keep the original warning unchanged. | Warning present in JSON, CLI text and library result (W-01); replaced when uplift is modelled (W-01a). |
| FR-16 to FR-19 | S3, S4 | Library and CLI deliverable (D5) | API and CLI tests |
| NFR-01 to NFR-04 | S2, S3 | Trust and API quality | numerical, compile-fail, determinism tests |
| NFR-05 to NFR-07 | S4 | Maintainability | build matrix, CMake graph, matrix review |

## 8. Gate decisions (owner)
| # | Decision | Effect |
|---|----------|--------|
| R1 | Friction is input as the friction angle phi in degrees, as in EM 1110-2-2200. | FR-10 and the JSON schema use `friction_angle_deg`; mu = tan(phi) is computed internally and reported. |
| R2 | Clockwise vertex order is rejected with an error message, not normalised. | FR-02. The error message states the detected orientation and expected CCW order. |
| R3 | **Verified 2026-11-01** against the official USACE publication (EM 1110-2-2200, 30 Jun 1995, 88 pages, `publications.usace.army.mil/Portals/76/Publications/EngineerManuals/EM_1110-2-2200.pdf`, the same edition the site serves today). The full text of the earlier third-party copy (damtoolbox.org) equals the official text word for word; the official file differs only by OCR text inside figures and appendices. Checked directly in the official file: Table 4-1 (zones middle 1/3, middle 1/2, within base; minimum sliding FS 2.0, 1.7, 1.3), Eqs. 4-1 to 4-3, para. 3-3.c(3) (tailwater, full depth for nonoverflow sections), para. 3-3.d(1) (uplift over 100 percent of the base; drain effectiveness 25 to 50 percent, up to 67 percent with CECW-ED approval; drains within 5 percent of the reservoir depth of the upstream face as one straight line; gallery above tailwater treated as the tailwater for the drain line; zero-compression zones) and the formulas of Figs. 3-2 and 3-3 (viewed as page images). All agree with the implementation. One difference found: extreme-condition bearing is 1.33 x allowable in Table 4-1, not the plain allowable (see section 2). | Section 2, docs/09. Citing the EM is now supported; a compliance claim still needs the engineer of record to confirm the inputs (phi, E, bearing). |
| R4 | Overturning stays informational, but the output carries every detail (all forces, arms and moments). | FR-11, NFR-09. |
