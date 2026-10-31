# ADR-0004: Virtual Load interface producing ForceContribution values

Status: ACCEPTED (owner approval, Phase 4b gate)
Requirements: FR-16, FR-21 (extension), D3, NFR-09

## Context
Uplift, tailwater, sediment and hydrodynamic loads are deferred but certain to come. Resultant, overturning report and checks must not change when a load is added.

## Options
1. **Closed set (`std::variant` of known loads).** Compiler checks exhaustive handling; adding a load edits the core; developers cannot add their own.
2. **Virtual `Load` interface** with `contributions(const Scenario&)`, held by the `Analyzer`. Adding a load adds a class; developers (S3) can supply loads without touching the core.
3. **Template/concept based.** Zero overhead; harder API for a library consumed across a JSON boundary, and no runtime composition.

## Decision (proposed)
Option 2. Performance is irrelevant at this scale (NFR-08 is a few milliseconds), and runtime composition fits the CLI (select loads from the scenario in later versions).

## Consequences
- Everything downstream of the loads works on `std::vector<ForceContribution>` only.
- Each `ForceContribution` carries a source name, so reports stay traceable.
- Loads must be pure and deterministic (NFR-03); this is documented on the interface and tested with a stub load.
