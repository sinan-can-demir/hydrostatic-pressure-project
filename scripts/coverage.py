#!/usr/bin/env python3
"""Line-coverage summary from gcov (test plan section 7; advisory, not a merge gate).

Build and run the suite with the coverage preset first:
    cmake --preset coverage && cmake --build build-coverage -j && ctest --test-dir build-coverage
    scripts/coverage.py [build-coverage]

Counts are merged across every translation unit that compiled a file, so a header used by many
tests is judged on all of them together. Only our own sources are reported (computation/, io/src,
apps/cli); tests, Catch2 and nlohmann/json are excluded. The v1 target is 90 percent of lines
for the hp_core and hp_math layers.
"""
import collections
import gzip
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
BUILD = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build-coverage"
TARGET = 90.0

LAYERS = [  # (name, path prefix relative to the repository, counts toward the target)
    ("hp_math", "computation/math/", True),
    ("hp_core", "computation/model/", True),
    ("hp_core", "computation/simulation/", True),
    ("hp_core", "computation/materials/", True),
    ("hp_units", "computation/units/", False),
    ("hp_io", "io/", False),
    ("hp_cli", "apps/cli/", False),
]


def layer_of(rel):
    for name, prefix, counted in LAYERS:
        if rel.startswith(prefix):
            return name, counted
    return None


def gcov_json(gcda):
    out = subprocess.run(
        ["gcov", "--json-format", "--stdout", str(gcda)],
        cwd=gcda.parent, capture_output=True, check=False,
    ).stdout
    if not out:
        return []
    # --stdout emits one JSON document per input file; older versions gzip it.
    try:
        text = gzip.decompress(out).decode()
    except OSError:
        text = out.decode()
    return [json.loads(line) for line in text.splitlines() if line.strip()]


def main():
    gcdas = sorted(BUILD.rglob("*.gcda"))
    if not gcdas:
        print(f"no .gcda files under {BUILD}; build with the coverage preset and run ctest first")
        return 2

    hits = collections.defaultdict(lambda: collections.defaultdict(int))  # file -> line -> count
    for gcda in gcdas:
        for doc in gcov_json(gcda):
            for entry in doc.get("files", []):
                path = pathlib.Path(entry["file"])
                if not path.is_absolute():
                    path = (gcda.parent / path).resolve()
                try:
                    rel = path.resolve().relative_to(ROOT).as_posix()
                except ValueError:
                    continue
                if layer_of(rel) is None:
                    continue
                for line in entry["lines"]:
                    hits[rel][line["line_number"]] += line["count"]

    totals = collections.defaultdict(lambda: [0, 0])  # layer -> [covered, total]
    print(f"{'file':52} {'lines':>6} {'cover':>7}")
    for rel in sorted(hits):
        lines = hits[rel]
        covered = sum(1 for c in lines.values() if c > 0)
        name, _ = layer_of(rel)
        totals[name][0] += covered
        totals[name][1] += len(lines)
        flag = "" if covered == len(lines) else "  uncovered: " + ",".join(
            str(n) for n in sorted(lines) if lines[n] == 0)[:60]
        print(f"{rel:52} {len(lines):6} {100.0 * covered / len(lines):6.1f}%{flag}")

    print()
    for name in dict.fromkeys(n for n, _, _ in LAYERS):
        covered, total = totals[name]
        if total == 0:
            continue
        pct = 100.0 * covered / total
        counted = any(c for n, _, c in LAYERS if n == name)
        mark = ""
        if counted:
            mark = f"  (target {TARGET:.0f}%: {'met' if pct >= TARGET else 'NOT met'})"
        print(f"{name:10} {covered:5}/{total:<5} {pct:6.1f}%{mark}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
