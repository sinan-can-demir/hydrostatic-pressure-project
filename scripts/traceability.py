#!/usr/bin/env python3
"""Requirements-to-tests traceability check (NFR-07, test plan section 8).

Reads every requirement row (ID and priority) from docs/02-requirements.md, then looks for
the ID in the test sources under tests/ (Catch2 tags, test names, CMake test names and the
comments beside them). Exits 1 if a Must (M) requirement has no test mention, except those
listed in VERIFIED_ELSEWHERE, which are checked by means other than a test tag.

Usage: scripts/traceability.py [--all]    (--all also lists the covered requirements)
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

# Must requirements that no test tag can express, with how they are verified instead.
VERIFIED_ELSEWHERE = {
    "NFR-05": "builds and passes on GCC and Clang in CI (.github/workflows/ci.yml), warnings as errors",
    "NFR-07": "this script",
}

ROW = re.compile(r"^\|\s*((?:FR|NFR)-\d+)\s*\|\s*([MSCW])\s*\|")


def requirements():
    found = {}
    for line in (ROOT / "docs" / "02-requirements.md").read_text().splitlines():
        m = ROW.match(line)
        if m:
            found[m.group(1)] = m.group(2)
    return found


def test_text():
    parts = []
    for path in sorted((ROOT / "tests").rglob("*")):
        if path.suffix in {".cpp", ".txt"} and path.is_file():
            parts.append(path.read_text())
    return "\n".join(parts)


def main():
    reqs = requirements()
    text = test_text()
    show_all = "--all" in sys.argv

    missing = []
    for rid, prio in reqs.items():
        # \b stops FR-1 matching FR-10; the lookahead stops FR-01 matching FR-010.
        count = len(re.findall(rf"\b{rid}\b(?!\d)", text))
        status = "covered" if count else "NO TEST"
        if not count and rid in VERIFIED_ELSEWHERE:
            status = "verified elsewhere: " + VERIFIED_ELSEWHERE[rid]
        elif not count and prio == "M":
            missing.append(rid)
        if show_all or status != "covered":
            print(f"{rid:8} [{prio}] {status}" + (f" ({count} mentions)" if count else ""))

    by_prio = {p: [r for r, q in reqs.items() if q == p] for p in "MSCW"}
    print(f"\n{len(reqs)} requirements: " + ", ".join(f"{len(v)} {p}" for p, v in by_prio.items()))
    if missing:
        print("UNCOVERED Must requirements: " + ", ".join(missing))
        return 1
    print("every Must requirement has at least one test")
    return 0


if __name__ == "__main__":
    sys.exit(main())
