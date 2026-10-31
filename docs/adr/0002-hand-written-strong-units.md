# ADR-0002: Hand-written strong unit types

Status: ACCEPTED (owner approval, Phase 4b gate)
Requirements: NFR-02, NFR-01

## Context
The repository already has strong wrappers (`Length`, `Pressure`, ...) with explicit constructors and deleted nonsense operators. The domain needs about ten quantities and a handful of derived operations (pressure x length gives force per width, force x length gives moment per width, etc.).

## Options
1. **Extend the hand-written types.** Small, readable, exact control of which operations exist; each new operation is written and tested by hand.
2. **mp-units or Boost.Units.** Full dimensional analysis automatically; heavy dependency, long compile errors, steep learning curve for a student-facing project.

## Decision (proposed)
Option 1. The set of quantities is small and fixed, and the owner wants to understand every step. Each allowed operation is a named, tested function; each forbidden one is `= delete` with a compile-fail test. Revisit if the number of quantities grows beyond roughly 15 or 3D work needs tensors.

## Consequences
- No dependency; fast compiles.
- Dimensional mistakes are caught only for operations we wrote; the compile-fail test list is the safety net.
- `Pressure * Length -> ForcePerWidth` is allowed (domain decision M1); `Pressure / Length` stays deleted.
