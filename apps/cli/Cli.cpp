#include "Cli.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>

#include "io/Json.hpp"
#include "simulation/Analyzer.hpp"

namespace cli {

namespace {

constexpr int kOk = 0;
constexpr int kInvalidInput = 1;
constexpr int kUsageOrFile = 2;

constexpr const char* kUsage =
    "Usage: hp_cli [input.json | -] [-o output.json]\n"
    "\n"
    "Analyses a gravity-dam section from a JSON scenario and writes the JSON result.\n"
    "Reads standard input when no input file (or '-') is given; writes standard output\n"
    "unless -o is given.\n"
    "\n"
    "Exit codes: 0 success (including a FAIL verdict), 1 invalid scenario, 2 usage or file error.\n";

} // namespace

int run(const std::vector<std::string>& args, std::istream& in, std::ostream& out, std::ostream& err) {
    std::optional<std::string> input_path;
    std::optional<std::string> output_path;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "-h" || arg == "--help") {
            out << kUsage;
            return kOk;
        }
        if (arg == "-o" || arg == "--output") {
            if (i + 1 >= args.size()) {
                err << "error: " << arg << " needs a file name\n" << kUsage;
                return kUsageOrFile;
            }
            output_path = args[++i];
        } else if (arg != "-" && arg.starts_with("-")) {
            err << "error: unknown option '" << arg << "'\n" << kUsage;
            return kUsageOrFile;
        } else if (input_path) {
            err << "error: more than one input file given\n" << kUsage;
            return kUsageOrFile;
        } else {
            input_path = arg;
        }
    }

    std::string text;
    if (input_path && *input_path != "-") {
        std::ifstream file(*input_path, std::ios::binary);
        if (!file) {
            err << "error: cannot read '" << *input_path << "'\n";
            return kUsageOrFile;
        }
        text.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    } else {
        text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    const auto scenario = io::parse_scenario(text);
    if (!scenario) {
        for (const io::IoError& e : scenario.error()) {
            err << "error: " << io::to_string(e.code) << " at " << (e.path.empty() ? "(document)" : e.path)
                << ": " << e.message << "\n";
        }
        return kInvalidInput;
    }

    const auto result = simulation::analyze(*scenario);
    if (!result) {
        for (const simulation::ValidationError& e : result.error()) {
            err << "error: " << simulation::to_string(e.code) << " at " << e.path << ": " << e.message
                << "\n";
        }
        return kInvalidInput;
    }

    const std::string json = io::result_to_json(*result);
    if (output_path) {
        std::ofstream file(*output_path, std::ios::binary);
        if (!file) {
            err << "error: cannot write '" << *output_path << "'\n";
            return kUsageOrFile;
        }
        file << json;
        if (!file) {
            err << "error: failed while writing '" << *output_path << "'\n";
            return kUsageOrFile;
        }
    } else {
        out << json;
    }
    return kOk;
}

} // namespace cli
