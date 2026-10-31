# ADR-0003: 2D analysis per unit width with 2D point types

Status: ACCEPTED (owner decision M3, Phase 4a gate)
Requirements: FR-01, scope D2

## Context
Industry practice for preliminary gravity-dam stability is a 2D cross-section per unit width. The repository has a `Vector3` stub.

## Decision
v1 uses `Point2` and `Vector2`. `Vector3.hpp` remains a stub reserved for later 3D work. The 3D-ready intent is carried by the `Load` and `ForceContribution` abstractions, not by an always-zero z coordinate.

## Consequences
- Simpler code and tests in v1.
- A later 3D version adds new types and an additional analyzer rather than modifying v1 code; the I/O schema is versioned for this reason (FR-19).
