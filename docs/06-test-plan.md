# 06 Test Strategy and Verification Plan

Status: APPROVED (Phase 5 gate), see section 10.
Inputs: `02-requirements.md`, `03-use-cases.md`, `04-domain-model.md`, `05-architecture.md`, ADR-0006 (Catch2), ADR-0007 (numeric policy).

## 1. Principles
- **Tests first (TDD).** In each Phase 6 slice the tests for its requirements are written and seen to fail before the code is written.
- **Independent oracles.** Expected values come from hand calculation or closed-form formulas, never from the engine's own output. Section 3 lists the reference cases; their numbers were computed separately from the engine design.
- **Every requirement is traceable.** Test names or Catch2 tags carry requirement IDs, for example `TEST_CASE("water force on vertical face", "[FR-06]")`, so section 8 can be checked mechanically.
- **Failures are specific.** One behaviour per test, with the inputs and expected value visible in the test.

## 2. Test levels
| Level | Scope | Target | Examples |
|-------|-------|--------|----------|
| L1 Unit | One function or type | `hp_units`, `hp_math`, geometry helpers | polygon area and centroid, orientation test, unit operators |
| L2 Component | Several units together | loads, validator, resultant, checks | `WaterLoad` on an inclined face; `Validator` error list |
| L3 Reference | Whole `analyze` on hand-solved cases | `hp_core` | AC-1 to AC-6 in section 3 |
| L4 Interface | I/O and CLI | `hp_io`, `hp_cli` | schema parse errors, golden JSON output, exit codes |
| L5 Compile-fail | Unit safety | `hp_units` | forbidden operations do not compile |
| L6 Property and robustness | Invariants, bad input | `hp_core`, `hp_io` | scaling laws, vertex rotation, NaN input |
| L7 Non-functional | Determinism, performance, sanitizers | all | repeat-run equality, ASan/UBSan, benchmark |

## 3. Reference cases (independent oracles)
Common inputs unless stated: rho = 1000 kg/m3, g = 9.81 m/s2, gamma = 23 500 N/m3, phi = 35 degrees (tan = 0.700208), usual load condition. The resultant location x_R is measured from the toe; the middle third of a base B is x_R in [B/3, 2B/3].

| Case | Section (CCW vertices, m) | Hw (m) | Expected |
|------|---------------------------|--------|----------|
| AC-1 rectangle, water at crest | (0,0) (5,0) (5,10) (0,10) | 10 | Fx = 490 500 N/m at 3.3333 m; W = 1 175 000; M_res = 2 937 500; M_over = 1 635 000; ratio 1.7966; x_R = 1.10851 (< B/3 = 1.6667); e = 1.39149; contact a = 3.32553, toe pressure about 706 650 Pa; sliding FS 1.6774. Usual: location FAIL, sliding FAIL, overall FAIL. Extreme: location pass, sliding pass (1.6774 >= 1.3). |
| AC-2 triangle, vertical upstream face | (0,0) (7,0) (0,10) | 10 | W = 822 500 at x = B/3 from heel; M_res = 3 838 333; M_over = 1 635 000; ratio 2.3476; x_R = 2.67882 (inside [2.3333, 4.6667]); e = 0.82118 (< B/6 = 1.1667) so full contact, V/B = 117 500; toe about 200 204 Pa, heel about 34 796 Pa; sliding FS 1.17415. Usual: location pass, sliding FAIL, overall FAIL. |
| AC-3 inclined upstream face | (0,0) (5,0) (5,10) (2,10) | 10 | Fx = 490 500 at 3.3333 m; Fy = 98 100 N/m downward (weight of the wedge of water above the face, area 10 m2) acting at x = 0.6667 m from the heel; area 40 m2, W = 940 000. |
| AC-4 partial level, passing case | same as AC-1 | 6 | Fx = 176 580 at 2.0 m; x_R = 2.19944 (inside [1.6667, 3.3333]); sliding FS 4.6593. Usual: all checks pass, overall PASS. |
| AC-5 resultant outside base | (0,0) (1,0) (1,10) (0,10) | 10 | W = 235 000, M_res = 117 500, M_over = 1 635 000, x_R = -6.45745 so the resultant lies beyond the heel: unstable, no valid contact, overall FAIL. |
| AC-6 empty reservoir | same as AC-1 | 0 | Fx = 0, x_R = B/2 = 2.5, full uniform contact V/B = 235 000 Pa, sliding reported as "no driving shear", overturning ratio unbounded, overall PASS. |

Independent verification: the numbers above were computed with a separate script (not engine code). The test files carry the hand derivation as comments, so a reader can check every oracle.

## 4. Test catalogue
IDs are tags used in test names. Each row lists the behaviours to cover.

| Area | Tag | Tests | Requirements |
|------|-----|-------|--------------|
| Units | U-01 | `ForcePerWidth`, `MomentPerWidth`, `Area`, `UnitWeight`, `Angle` construct and expose correct SI values; arithmetic on like units; `hydrostatic()` gives 9810 Pa for 1 m of water | NFR-02 |
| Units | U-02 | `Pressure * Length` returns `ForcePerWidth` with the right value (decision M1) | NFR-02, FR-06 |
| Math | M-01 | `Point2`/`Vector2`: components, point minus point is a vector, vector arithmetic, cross product sign (CCW positive), dot product | FR-01, FR-02 |
| Model | S-01 | `Fluid`, `Concrete` defaults and overrides; `Scenario` holds AC-1 inputs and applies documented defaults; `CriteriaSet::usace()` equals Table 4-1 | FR-03, FR-13, FR-14 |
| Geometry | G-01 | Area and centroid of rectangle, triangle, trapezoid vs closed form | FR-07 |
| Geometry | G-02 | Heel, toe, crest height, base length extracted correctly | FR-01 |
| Geometry | G-03 | Orientation: CCW accepted, CW rejected with orientation message | FR-02, R2 |
| Geometry | G-04 | Simple polygon test: bow-tie rejected; touching and collinear edge cases | FR-02 |
| Geometry | G-05 | Base edge rules: none on y = 0, two separate base edges, vertex below base, sloped bottom | FR-02 |
| Geometry | G-06 | Upstream face dips in elevation from heel to crest: rejected (M2) | FR-02 |
| Validation | V-01 | Each invalid input yields its own error code and field path | FR-02 to FR-04 |
| Validation | V-02 | Several simultaneous errors are all reported (U1) | UC-09 |
| Validation | V-03 | Water level above crest rejected; water level 0 accepted; level equal to crest accepted | FR-04, CON-04 |
| Validation | V-04 | Friction angle 0, 90, negative, NaN rejected; 0.001 and 89.999 accepted (U3) | UC-02 |
| Validation | V-05 | NaN, infinity and magnitude beyond 1e6 m rejected | NFR-10 |
| Loads | L-01 | `WaterLoad`: Fx, line of action, pressure samples for vertical face (AC-1, AC-4) | FR-05, FR-06 |
| Loads | L-02 | `WaterLoad` on inclined face gives Fy and its point (AC-3); clipping when Hw is below the crest | FR-06 |
| Loads | L-03 | `SelfWeight`: W and centroid (AC-1, AC-2, AC-3) | FR-07 |
| Loads | L-04 | A stub `Load` added to `Analyzer` appears in contributions and shifts the resultant (proves the extension seam) | ADR-0004 |
| Resultant | R-01 | V, H, M, x_R, e for AC-1 to AC-6 | FR-08 |
| Resultant | R-02 | Contact: full (AC-2), partial near toe (AC-1), partial near heel (mirrored case), none (AC-5); pressures vs formulas | FR-09 |
| Resultant | R-03 | Overturning report lists every contribution with arm, moment and role; totals match R-01; ratio unbounded when no overturning moment | FR-11 |
| Checks | C-01 | Sliding FS; `H = 0` gives "no driving shear" | FR-10 |
| Checks | C-02 | Boundary behaviour: x_R exactly at B/3 and 2B/3 passes usual; one ulp outside fails; FS exactly at minimum passes, one ulp below fails | FR-12, ADR-0007 |
| Checks | C-03 | Presets equal Table 4-1 for all three conditions; user override replaces them; invalid override rejected | FR-13 |
| Checks | C-04 | Bearing check: pass, fail, and skipped when not supplied (skipped is not pass) | FR-14, U2 |
| Checks | C-05 | Overall verdict: FAIL if any check fails; skipped checks do not fail | U2 |
| Warnings | W-01 | The uplift warning is present in every result, including failed ones, in the library result, JSON and CLI text | FR-15 |
| API | A-01 | `analyze` is callable without files; no exceptions on any invalid scenario (run the whole invalid-input table under a no-throw assertion) | FR-16, NFR-04 |
| API | A-02 | Same scenario twice gives equal `Result` | FR-16, NFR-03 |
| I/O | I-01 | AC-1 JSON parses to the expected `Scenario`; defaults applied for omitted optionals | FR-17, FR-03 |
| I/O | I-02 | Unknown key, wrong type, missing required field, malformed JSON, unknown `schema_version` each give a clear error | FR-17, FR-19 |
| I/O | I-03 | Golden-file: AC-1 result JSON equals the stored file (key order, number format fixed) | FR-17 |
| I/O | I-04 | Result echoes inputs and criteria; re-running the echoed input reproduces the result | UC-07, NFR-03 |
| I/O | I-05 | `pressure_samples` yields that many evenly spaced samples | FR-20 |
| CLI | CLI-01 | Success: exit 0, output to file and to stdout; input from file and stdin | FR-17, FR-18 |
| CLI | CLI-02 | Invalid scenario: non-zero exit, all errors on stderr, no result written | FR-18, U1 |
| CLI | CLI-03 | Unreadable input, unwritable output path: non-zero exit with a message | UC-07 |
| Compile-fail | CF-01 | Each forbidden operation fails to compile (list in section 5) | NFR-02 |
| Property | P-01 | Scale all lengths by k: Fx, W scale by k^2, x_R by k, FS and overturning ratio unchanged | NFR-01 |
| Property | P-02 | Cyclic rotation of the vertex list gives an identical result | FR-01 |
| Property | P-03 | Translating the section along x leaves all results unchanged | FR-08 |
| Property | P-04 | Increasing water level never decreases Fx or the overturning moment (monotonicity) | FR-06 |
| Property | P-05 | Random valid sections: sum of contributions equals V, H, M reported; contact pressures integrate to V | FR-08, FR-09 |
| Non-functional | N-01 | Bitwise-identical output over 1000 repeated runs | NFR-03 |
| Non-functional | N-02 | Whole suite passes under AddressSanitizer and UBSan | NFR-03 |
| Non-functional | N-03 | 1000-vertex section analysed in under 10 ms (advisory benchmark, not a gating test) | NFR-08 |
| Non-functional | N-04 | Build graph: `hp_core` fails to compile if it includes an `hp_io` or JSON header (negative build test) | NFR-06 |

## 5. Compile-fail list (CF-01)
Each snippet must fail to compile; an accompanying positive snippet proves the harness itself works.
`Pressure / Length`; `Pressure + Length`; `Length + Density`; `Pressure * Pressure`; `Density * Length` (not a defined physical operation here); constructing any unit from a bare `double` without the explicit constructor (implicit conversion); passing `Pressure` where `Length` is expected; `ForcePerWidth + MomentPerWidth`. Allowed counterparts (`Pressure * Length`, `Pressure + Pressure`, `ForcePerWidth * Length`) are tested to compile.

## 6. Tolerances
Per ADR-0007 and NFR-01: reference comparisons use relative error 1e-9 (absolute 1e-9 where the expected value is zero). Boundary tests (C-02) use exact thresholds and one-ulp neighbours, not tolerances. Property tests use 1e-9 relative.

## 7. Tooling and workflow
- **Framework:** Catch2 v3 (ADR-0006). If it is new to you, the three things you need: `TEST_CASE("name", "[tag]")` defines a test; `REQUIRE(expr)` checks it and stops that test on failure; `REQUIRE_THAT(x, Catch::Matchers::WithinRel(expected, 1e-9))` compares doubles with a relative tolerance. `SECTION`s inside a test share setup, and `GENERATE(...)` runs a test over a table of values.
- **Runner:** CTest. `ctest --output-on-failure` runs everything; `ctest -L fast` runs L1 to L4, and slower groups (L6 random sweeps, N-01, N-02, N-03) carry a `slow` label.
- **Sanitizers:** a separate CMake preset `asan-ubsan`, run before each slice is closed.
- **Compile-fail and negative build tests:** CMake `try_compile` or targets marked `WILL_FAIL`, independent of Catch2.
- **Coverage:** gcov/lcov report, advisory. Target: 90 percent line coverage for `hp_core` and `hp_math` by the end of v1; not a merge gate, because coverage does not prove correctness.
- **Warnings:** `-Wall -Wextra -Wpedantic -Werror` on all targets, tests included.

## 8. Traceability matrix (requirements to tests)
| Requirement | Tests |
|-------------|-------|
| FR-01 | M-01, G-02, P-02, I-01 |
| FR-02 | G-03, G-04, G-05, G-06, V-01 |
| FR-03 | S-01, V-01, I-01 |
| FR-04 | V-03 |
| FR-05 | L-01 |
| FR-06 | U-02, L-01, L-02, P-04 |
| FR-07 | G-01, L-03 |
| FR-08 | R-01, P-03, P-05 |
| FR-09 | R-02, P-05 |
| FR-10 | C-01 |
| FR-11 | R-03 |
| FR-12 | C-02, C-05 |
| FR-13 | S-01, C-03 |
| FR-14 | C-04 |
| FR-15 | W-01 |
| FR-16 | A-01, A-02 |
| FR-17 | I-01, I-02, I-03, CLI-01 |
| FR-18 | CLI-01, CLI-02 |
| FR-19 | I-02, docs/07-json-schema.md |
| FR-20 | I-05 |
| NFR-01 | AC-1 to AC-6 (all reference tests), P-01 |
| NFR-02 | U-01, U-02, CF-01 |
| NFR-03 | A-02, I-04, N-01, N-02 |
| NFR-04 | A-01, V-01 |
| NFR-05 | build and full suite on GCC 14 and Clang 19 in CI (`.github/workflows/ci.yml`, docs/08); locally also `cmake --preset clang` |
| NFR-06 | N-04 |
| NFR-07 | this matrix; `scripts/traceability.py` checks that every M requirement ID appears in at least one test |
| NFR-08 | N-03 (`hp_bench_analyze`, advisory; run in a Release build) |
| NFR-09 | R-03, I-04 |
| NFR-10 | V-05 |
| NFR-11 | review of committed config |
All M-priority requirements are covered. FR-21 (won't) has no tests, except L-04 which proves the extension seam.

## 9. Exit criteria per Phase 6 slice
A slice is complete when: its tests were written first and failed; all its tests pass; the whole suite still passes, including the sanitizer preset; the warnings-as-errors build is clean; the traceability script finds no uncovered M requirement for the slice; and the slice is committed with its test IDs in the message.

## 10. Gate decisions (owner)
| # | Decision | Notes |
|---|----------|-------|
| T1 | Golden-file tests approved. JSON is the v1 output format; a more efficient storage format may replace or complement it later. | Format code lives only in `hp_io` (ADR-0005), so a second serializer can be added behind the same `Result` type without touching the core. The golden files belong to the JSON serializer; a new format brings its own golden files. Recommendation: keep JSON as the human-readable interchange format even after a binary one exists. |
| T2 | Coverage is advisory: a report showing which lines the tests executed, with a 90 percent goal for the core. It never blocks a commit. | Coverage only shows what code ran, not that it was checked correctly; the reference cases and properties are what prove correctness. |
| T3 | No external oracle available; v1 relies on AC-1 to AC-6. | Residual risk: all oracles come from the same author and formulas. Mitigation: when a published textbook or USACE example is found, add it as AC-7 and rerun. |

## 11. Increment v1.1: tailwater and uplift (docs/09, ADR-0008)
Status: ACCEPTED with the increment gate. Tests are written first, slice by slice (U1 to U4). Requirement rows enter `02-requirements.md` in the slice whose tests cover them.

### 11.1 Reference cases (independent oracle)
Common inputs: the AC-1 dam (rectangle B = 5, H = 10, Hw = 10), rho = 1000, g = 9.81, gamma = 23 500, phi = 35 degrees, usual condition. W = 1 175 000, M_res = 2 937 500, water Fx = 490 500, M_water = -1 635 000. Hand derivations are in `docs/09` section 6; all five were re-derived by a separate numerical-integration script (midpoint rule, 200 000 steps, no engine formulas), which also caught an arithmetic error in the first hand value of AU-3.

| Case | Added input | Uplift U (N/m) | Uplift moment | V | H | M about toe | x_R (m) | Sliding FS |
|------|-------------|----------------|---------------|---|---|-------------|---------|-----------|
| AU-1 | uplift, no drains, no tailwater | 245 250 | -817 500 | 929 750 | 490 500 | 485 000 | 0.5216 | 1.3273 |
| AU-2 | tailwater 3 m (Fx = -44 145 at 1 m, moment +44 145), uplift | 318 825 | -940 125 | 856 175 | 446 355 | 406 520 | 0.4748 | 1.3431 |
| AU-3 | drains E = 0.5, X = 1 m | 147 150 | -523 200 | 1 027 850 | 490 500 | 779 300 | 0.7582 | 1.4673 |
| AU-5 | drains E = 0.5, X = 0.2 m (Fig. 3-3 rule) | 122 625 | -408 750 | 1 052 375 | 490 500 | 893 750 | 0.8493 | 1.5023 |

AU-4 (cracked base) is reserved for tier B. Cases AU-1, AU-2 and AU-3 also have contact length 3 x_R < B, so the FR-28 warning must appear (AU-1 to AU-3 and AU-5 are all partial-contact cases).

### 11.2 Test catalogue additions
| Area | Tag | Tests | Requirements |
|------|-----|-------|--------------|
| Validation | TW-01 | Tailwater depth: negative, NaN, above Hw rejected, each with its own code and path; 0 and exactly Hw accepted; omitted means none | FR-22 |
| Validation | TW-02 | Downstream face that rises toward the toe is rejected; vertical and sloping faces accepted | FR-22 (domain invariant 3) |
| Loads | TW-03 | `TailwaterLoad` on a vertical face: Fx = 1/2 rho g H2^2 upstream-directed at H2/3 (AU-2); H2 = 0 emits nothing; scenario without tailwater leaves the contribution list unchanged | FR-23, NFR-12 |
| Loads | TW-04 | `TailwaterLoad` on a sloped downstream face: Fy equals the weight of water above the face and acts downward at the right x | FR-23 |
| Validation | UP-01 | Drains: E <= 0, E > 1, NaN, X < 0, X > B, negative gallery elevation each rejected with distinct codes; no drains block means E = 0 | FR-25 |
| Loads | UP-02 | `UpliftLoad` without drains: heel/toe ordinates, force and moment (AU-1, AU-2); tailwater changes only the toe ordinate | FR-24 |
| Loads | UP-03 | `UpliftLoad` with drains: drain-line head H3 for H4 > H2 and H4 < H2 (gallery rule); AU-3 | FR-24, FR-25 |
| Loads | UP-04 | Drains within 0.05 H1 of the upstream face use the single-line rule (AU-5); X just above and just below the threshold select the two rules | FR-24 |
| Property | UP-05 | Random valid sections and drain data: the uplift force and moment equal an independent numerical integration of the diagram (as P-05) | FR-24 |
| Property | UP-06 | Monotonicity: more drain effectiveness never increases U; higher tailwater never increases U | FR-24, FR-25 |
| Pipeline | UP-07 | AU-1 to AU-3 and AU-5 through `analyze`: V, H, M, x_R, FS match 11.1; report lists "uplift" and "tailwater" terms with arm, moment and role | FR-26, FR-27 |
| Warnings | UP-08 | E at 0.24, 0.25, 0.50, 0.51, 0.67, 0.68 produce the documented warnings; E is never altered | FR-25 |
| Warnings | UP-09 | FR-28: contact length < B adds the unconservative-uplift warning; full contact does not; U > W reports unstable | FR-28 |
| Warnings | W-01a | Without uplift the v1 warning is present (W-01 unchanged); with uplift it is replaced by the model statement | FR-15 |
| Samples | UP-10 | Uplift samples include heel, drain line and toe with the right pressures | FR-30 |
| I/O | I-06 | Schema version 1 documents behave exactly as before (golden `ac1_result.json` unchanged); version 2 with tailwater and drains parses; unknown keys still rejected; version 3 rejected | FR-31, FR-19, NFR-12 |
| I/O | I-07 | Golden file for AU-3 result JSON | FR-31 |
| Regression | N-05 | Every v1 test passes unchanged, and AC-1 to AC-6 results are bitwise equal with the new loads registered but inactive | NFR-12 |

### 11.3 Traceability additions
| Requirement | Tests |
|-------------|-------|
| FR-22 | TW-01, TW-02 |
| FR-23 | TW-03, TW-04 |
| FR-24 | UP-02, UP-03, UP-04, UP-05, UP-06 |
| FR-25 | UP-01, UP-03, UP-06, UP-08 |
| FR-26 | UP-07 |
| FR-27 | UP-07 |
| FR-28 | UP-09 |
| FR-30 | UP-10 |
| FR-31 | I-06, I-07 |
| NFR-12 | TW-03, N-05, I-06 |
FR-29 (cracked base, tier B) has no tests until it is scheduled. FR-15 gains W-01a.

### 11.4 Slice exit criteria
Section 9 applies to each of U1 to U4. In addition: the oracle script results in 11.1 appear as comments in the tests, and the mutation check (as for S7) is repeated on `TailwaterLoad` and `UpliftLoad` before U3 closes.
