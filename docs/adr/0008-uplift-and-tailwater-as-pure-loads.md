# ADR-0008: Uplift and tailwater as pure loads, schema version 2

Status: ACCEPTED (increment v1.1, `docs/09-uplift-tailwater-requirements.md`)
Requirements: FR-22 to FR-31, NFR-12; builds on ADR-0004

## Context
Uplift and tailwater are the first loads added after v1. Uplift acts on the base and in tier A (full base in compression) depends only on the scenario: heads, base length and drain data. Tailwater acts on the downstream face. A later tier B (cracked base) makes uplift depend on the contact length, which is a result.

## Decision
1. `TailwaterLoad` and `UpliftLoad` are ordinary `Load` implementations (ADR-0004). `analyze()` always includes them; each returns no contributions when the scenario has no tailwater depth (and, for uplift, no uplift request). This is what keeps results for old scenarios bit-identical (NFR-12).
2. Uplift is requested explicitly by the scenario (an `uplift` block), not implied by tailwater, because uplift without tailwater is valid (toe head zero) and tailwater without uplift is a separate load.
3. The wetted-face integration used by `WaterLoad` is extracted and reused for the downstream face, not copied.
4. Tier B will not change tier A loads. It adds an iteration in `Analyzer` around the load evaluation, with its own ADR when scheduled.
5. The JSON `schema_version` becomes 2 and the parser accepts 1 and 2. Version-1 documents keep their exact output. Rationale: older parsers reject unknown keys, so a version number is the documented compatibility signal (FR-19).

## Consequences
- No change to resultant, contact, sliding, overturning report or checks (FR-26).
- The validator gains rules for tailwater, drains and the downstream-face invariant.
- One more version of the schema document to maintain (`docs/07`).
