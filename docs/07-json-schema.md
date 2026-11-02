# 07 JSON Schema, versions 1 and 2

Status: written in Phase 6 (slice S7), version 2 added in slice U4 (tailwater and uplift, docs/09). Documents what `hp_io` implements. Satisfies FR-19 and FR-31.
Sources: `io/include/io/Json.hpp`, `io/src/Json.cpp`, `apps/cli/Cli.hpp`.
Worked examples: `tests/data/ac1_scenario.json` and `ac1_result.json` (version 1), `tests/data/au3_scenario.json` and `au3_result.json` (version 2: uplift with drains).

## 1. Conventions
- Every key carries its unit as a suffix (`_m`, `_deg`, `_pa`, `_kg_m3`, `_m_s2`, `_n_m3`, `_n_m`, `_n_m_m`), so a file is self-describing. Force is per metre of width (N/m), moment is N*m/m.
- x runs downstream (upstream face on the left), y is up, the base lies on y = 0. A positive `fx` pushes downstream; a downward force has a negative `fy`. Moments are about the toe, counter-clockwise positive: resisting moments are positive, overturning moments negative.
- `schema_version` is an integer and is checked first. Supported values are `1` and `2`; anything else is rejected with `unsupported_schema_version` and nothing else is reported. Version 2 adds the keys marked "v2" below; in a version 1 document they are unknown keys, so version 1 files behave exactly as they always did.
- Unknown keys are rejected at every level (so a typo is never silently ignored). All structural problems found are reported together.
- JSON `null` for an optional field means "not given". Integers and decimals are both accepted for numeric fields.

## 2. Input: the scenario

| Key | Type | Required | Default | Meaning |
|-----|------|----------|---------|---------|
| `schema_version` | integer | yes | | `1` or `2`. |
| `dam.vertices_m` | array of `[x, y]` pairs | yes | | Counter-clockwise outline, at least 3 vertices, one horizontal edge on y = 0. |
| `water_level_m` | number | yes | | Upstream water elevation above the base, 0 to crest height. |
| `friction_angle_deg` | number | yes | | Foundation friction angle, strictly between 0 and 90. Cohesion is fixed at zero. |
| `load_condition` | string | no | `"usual"` | `"usual"`, `"unusual"` or `"extreme"`; selects the criteria row. |
| `fluid.density_kg_m3` | number | no | `1000` | Greater than zero. |
| `fluid.gravity_m_s2` | number | no | `9.81` | Greater than zero. |
| `concrete.unit_weight_n_m3` | number | no | `23500` | Greater than zero. |
| `allowable_bearing_pa` | number or null | no | `null` | Allowable base pressure; `null` skips the bearing check. Greater than zero when given. |
| `criteria` | object | no | USACE preset | See below. |
| `pressure_samples` | integer or null | no | `null` | Number of evenly spaced pressure samples along the wetted face, 2 to 100000; `null` samples the face vertices plus the water surface. |
| `tailwater_depth_m` | number or null | no (v2) | `null` | Downstream water depth above the base, 0 to the water level. `null` means no tailwater. |
| `uplift` | object or null | no (v2) | `null` | Present (even as `{}`) requests uplift on the base; `null` or absent means none. |

`uplift` keys: `drains` (object or null; `null` or absent means no drains), holding `effectiveness` (E, greater than 0 and at most 1, required: there is no default), `distance_from_heel_m` (X, 0 to the base length, required) and `gallery_elevation_m` (optional, default 0, not negative). Uplift is computed for a fully compressed base; the result warns when it is not (FR-28).

`criteria` keys: `preset` (only `"usace_em_1110_2_2200"`, EM 1110-2-2200 Table 4-1), and optional rows `usual`, `unusual`, `extreme`. A row has `zone` (`"middle_third"`, `"middle_half"` or `"within_base"`) and `min_sliding_fs` (greater than zero). Rows replace the preset field by field; omitted fields keep the preset value.

| Condition | Zone | Minimum sliding FS |
|-----------|------|--------------------|
| usual | middle_third | 2.0 |
| unusual | middle_half | 1.7 |
| extreme | within_base | 1.3 |

Minimal valid document:
```json
{ "schema_version": 1,
  "dam": { "vertices_m": [[0,0],[5,0],[5,10],[0,10]] },
  "water_level_m": 10,
  "friction_angle_deg": 35 }
```

## 3. Output: the result
Keys appear in this fixed order; numbers use the shortest text that reads back exactly, so equal results give identical files. The scenario is echoed in full so a result file documents its own inputs.

| Key | Contents |
|-----|----------|
| `schema_version` | `1`, or `2` when the scenario sets `tailwater_depth_m` or `uplift` (so version 1 scenarios keep their exact output) |
| `input` | The scenario in the input schema above, every value explicit (for version 2, `tailwater_depth_m` and `uplift` are written, `null` when absent; `gallery_elevation_m` is always written). Feeding `input` back to the tool reproduces the result exactly. |
| `pressure` | Array of `{x_m, y_m, pressure_pa, normal: [nx, ny]}` along the wetted face from the heel up. `normal` is the unit vector into the dam. Empty when the face is dry. |
| `uplift_pressure` | Version 2 results with uplift only. Array of `{x_m, pressure_pa}`: the break points of the uplift diagram from heel to toe (heel, the drain line when drains apply away from the heel, toe); straight lines between them reproduce the diagram. |
| `contributions` | Array of `{source, fx_n_m, fy_n_m, x_m, y_m}`, one per force acting on the dam. Water appears as `"water (horizontal)"` and `"water (vertical)"` (a component that is exactly zero is omitted), then `"self weight"`, then, in version 2, `"tailwater (horizontal)"`, `"tailwater (vertical)"` and `"uplift"` (the uplift is an upward force, positive `fy_n_m`, at the centroid of the diagram on y = 0). The point lies on the line of action. |
| `resultant` | `vertical_n_m` (V, downward positive), `horizontal_n_m` (H, downstream positive), `moment_about_toe_n_m_m`, `x_from_toe_m` (x_R) and `eccentricity_m` (e = B/2 - x_R, positive toward the toe), both `null` when V is not downward, and `base_length_m`. |
| `contact` | `state` (`"full"`, `"partial"`, `"none"`), `length_m`, `toe_pressure_pa`, `heel_pressure_pa`. `none` means the resultant is on or beyond the base ends. |
| `sliding` | `factor_of_safety` (V tan(phi) / H) or `null`, and `no_driving_shear` (true when H <= 0). |
| `overturning` | `terms` (array of `{source, direction, force_n_m, arm_m, moment_n_m_m, role}`; `direction` is `"horizontal"` or `"vertical"`, `role` is `"resisting"`, `"overturning"` or `"neutral"`), `resisting_moment_n_m_m`, `overturning_moment_n_m_m` (magnitude), `ratio` (resisting / overturning, `null` when there is no overturning moment) and `unbounded`. Informational, not a pass/fail criterion. |
| `checks` | `location` (`status`, `zone`, `lower_m`, `upper_m`: the allowed x_R range from the toe), `sliding` (`status`, `factor_of_safety`, `minimum`), `bearing` (`status`, `max_pressure_pa`, `allowable_pa`). `status` is `"pass"`, `"fail"` or `"skipped"`. |
| `warnings` | Array of strings. Without an uplift request it is exactly `"uplift not modelled; results are unconservative for real dams"`. With uplift it holds the model statement, then drain-effectiveness warnings (E outside 25 to 50 percent, or above 67 percent) and the partial-contact warning when the base is not fully in compression. |
| `verdict` | `"pass"` unless some check failed (skipped checks never fail), else `"fail"`. |

Check rules: a value exactly on a threshold passes. The location check also fails when there is no base contact. A sliding check with no driving shear passes. The bearing check is skipped when no allowable pressure was given and fails when one was given but there is no contact.

## 4. Errors
Parse errors (exit code 1) are printed as `error: <code> at <path>: <message>`; `<path>` is a dotted path such as `fluid.density_kg_m3` or `dam.vertices_m[2][0]`, or `(document)` for the whole file.

| Code | Meaning |
|------|---------|
| `malformed_json` | The text is not valid JSON. |
| `unsupported_schema_version` | `schema_version` is not `1` or `2`. |
| `missing_field` | A required key is absent. |
| `unknown_field` | A key the schema does not define. |
| `wrong_type` | A value of the wrong JSON type (for example a string where a number is required). |
| `invalid_value` | An unknown enumeration value (`load_condition`, `criteria.preset`, `zone`). |

If the document parses, the scenario is validated and every problem is reported together, in the same format. Codes are stable: `too_few_vertices`, `non_finite_value`, `value_out_of_range`, `duplicate_vertex`, `clockwise_order`, `zero_area`, `self_intersecting`, `no_base_edge`, `multiple_base_edges`, `vertex_below_base`, `upstream_face_dips`, `water_level_negative`, `water_level_above_crest`, `friction_angle_out_of_range`, `non_positive_fluid_density`, `non_positive_gravity`, `non_positive_unit_weight`, `non_positive_allowable_bearing`, `non_positive_sliding_fs`, `pressure_samples_out_of_range`. From version 2 (tailwater and uplift, docs/09): `downstream_face_rises`, `tailwater_negative`, `tailwater_above_water_level`, `drain_effectiveness_out_of_range`, `drain_distance_out_of_range`, `gallery_elevation_negative`.

## 5. Command line
`hp_cli [input.json | -] [-o output.json]` reads the file (or standard input when none is given or for `-`) and writes the result to standard output or to the `-o` file.

| Exit code | Meaning |
|-----------|---------|
| 0 | The analysis ran. A `"fail"` verdict is still a successful analysis. |
| 1 | The document is malformed or the scenario is invalid. All errors go to standard error; no output is written. |
| 2 | Usage error, or an input or output file cannot be read or written. |

## 6. Compatibility
Unknown keys are rejected, so a file that uses a newer key needs a tool that knows it. For that reason new optional keys bump the version (decision D-U7): version 2 added tailwater and uplift. A tool reads every version up to its own, and a document of version N accepts only the keys of version N. A scenario that does not use the new keys is written as version 1, byte for byte as before (NFR-12).
