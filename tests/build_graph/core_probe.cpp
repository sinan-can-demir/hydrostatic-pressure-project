// Build-graph probe (N-04, NFR-06). Compiled against hp_core only. With no macro it includes a
// core header and must compile (proves the harness works). Each CF_ macro adds one include the
// core must NOT be able to reach, and that variant must fail to compile.
#include "simulation/Analyzer.hpp"

#if defined(CF_IO_HEADER)
#include "io/Json.hpp"
#endif

#if defined(CF_JSON_LIBRARY)
#include <nlohmann/json.hpp>
#endif

int main() { return 0; }
