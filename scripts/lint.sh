#!/usr/bin/env bash
# Formatting and static-analysis check (NFR-11). Same command locally and in CI.
#   scripts/lint.sh            run both checks
#   scripts/lint.sh format     only clang-format (dry run, no files are changed)
#   scripts/lint.sh tidy       only clang-tidy on the non-test sources
# Override the tools with CLANG_FORMAT, CLANG_TIDY and CXX (CI uses the -19 versions);
# CMAKE_ARGS adds extra configure flags.
# Exit status is non-zero if either selected check reports anything.
set -u
cd "$(dirname "$0")/.."

CLANG_FORMAT=${CLANG_FORMAT:-clang-format}
CLANG_TIDY=${CLANG_TIDY:-clang-tidy}
mode=${1:-all}
status=0

if [[ $mode == all || $mode == format ]]; then
    echo "== clang-format ($($CLANG_FORMAT --version | head -1))"
    git ls-files '*.cpp' '*.hpp' | xargs "$CLANG_FORMAT" --dry-run -Werror || status=1
fi

if [[ $mode == all || $mode == tidy ]]; then
    echo "== clang-tidy ($($CLANG_TIDY --version | grep -m1 version))"
    # Only configures (writes compile_commands.json); nothing is built.
    cmake -S . -B build-lint -DCMAKE_CXX_COMPILER="${CXX:-clang++}" ${CMAKE_ARGS:-} >/dev/null || exit 2
    git ls-files 'computation/*.cpp' 'io/*.cpp' 'apps/*.cpp' |
        xargs "$CLANG_TIDY" -p build-lint --quiet --warnings-as-errors='*' || status=1
fi

exit $status
