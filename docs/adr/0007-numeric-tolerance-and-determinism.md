# ADR-0007: Numeric tolerance and determinism policy

Status: PROPOSED
Requirements: NFR-01, NFR-03, NFR-10

## Context
Closed-form integration in `double` gives results to about 1e-15 relative, but decisions (is the polygon simple, does the resultant sit exactly on a threshold) depend on comparisons, and results should not vary with compiler fused-multiply-add choices.

## Decision (proposed)
1. All computation in `double`; no `float`, no `long double`.
2. Compile with `-ffp-contract=off`, no `-ffast-math`.
3. One header defines the tolerance policy: geometric tests use a relative tolerance of 1e-12 times the section's bounding-box size; pass/fail checks compare without tolerance (a value exactly on the threshold passes, `>=` for minima, `<=` for bounds), so boundary behaviour is predictable and testable.
4. Inputs must be finite and within +/- 1e6 m (NFR-10); anything else is a validation error.

## Consequences
- Reference tests can demand 1e-9 relative error with large margin.
- Behaviour at exact thresholds is defined, and boundary tests flip exactly at the threshold (FR-12).
- Cross-compiler results may differ in the last bits; NFR-03 is scoped to the same platform and compiler.
