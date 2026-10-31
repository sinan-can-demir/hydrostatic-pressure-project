/**
 * @file Cli.hpp
 * @brief The command-line tool as a function of its arguments and streams, so it can be tested
 *        without spawning a process.
 *
 * Exit codes (FR-18): 0 the analysis ran (a FAIL verdict is still a successful analysis);
 * 1 the scenario is malformed or invalid, with every error on `err` and nothing written;
 * 2 usage error or a file that cannot be read or written.
 */

#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace cli {

int run(const std::vector<std::string>& args, std::istream& in, std::ostream& out, std::ostream& err);

} // namespace cli
