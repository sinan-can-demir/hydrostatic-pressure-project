#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "Cli.hpp"

namespace fs = std::filesystem;

namespace {

const std::string kDataDir = HP_TEST_DATA_DIR;

struct Outcome {
    int code;
    std::string out;
    std::string err;
};

Outcome run(const std::vector<std::string>& args, const std::string& stdin_text = "") {
    std::istringstream in(stdin_text);
    std::ostringstream out, err;
    const int code = cli::run(args, in, out, err);
    return {code, out.str(), err.str()};
}

std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// A scratch directory removed when the test ends. The name is unique per instance because CTest
// runs the tests in parallel processes; a shared name let one test delete another's directory.
struct TempDir {
    fs::path path;
    TempDir() {
        std::string pattern = (fs::temp_directory_path() / "hp_cli_tests_XXXXXX").string();
        REQUIRE(::mkdtemp(pattern.data()) != nullptr);
        path = pattern;
    }
    ~TempDir() { fs::remove_all(path); }
};

const std::string kAc1 = kDataDir + "/ac1_scenario.json";

} // namespace

TEST_CASE("CLI-01 reads a file and writes the result to stdout", "[FR-17][FR-18][CLI-01]") {
    const Outcome r = run({kAc1});
    CHECK(r.code == 0);
    CHECK(r.err.empty());
    CHECK(r.out == read_file(kDataDir + "/ac1_result.json"));
}

TEST_CASE("CLI-01 reads stdin when no input file or '-' is given", "[FR-17][CLI-01]") {
    const std::string doc = read_file(kAc1);
    const Outcome implicit = run({}, doc);
    const Outcome dash = run({"-"}, doc);
    CHECK(implicit.code == 0);
    CHECK(dash.code == 0);
    CHECK(implicit.out == read_file(kDataDir + "/ac1_result.json"));
    CHECK(dash.out == implicit.out);
}

TEST_CASE("CLI-01 -o writes the result to a file and leaves stdout empty", "[FR-17][CLI-01]") {
    TempDir tmp;
    const std::string target = (tmp.path / "result.json").string();
    const Outcome r = run({kAc1, "-o", target});
    CHECK(r.code == 0);
    CHECK(r.out.empty());
    CHECK(read_file(target) == read_file(kDataDir + "/ac1_result.json"));
}

TEST_CASE("CLI-01 a FAIL verdict is a successful analysis (exit 0)", "[FR-18][CLI-01]") {
    const Outcome r = run({kAc1}); // AC-1 usual fails its checks
    CHECK(r.code == 0);
    CHECK(nlohmann::json::parse(r.out)["verdict"] == "fail");
}

TEST_CASE("CLI-02 an invalid scenario exits 1 with every error on stderr and no output", "[FR-18][CLI-02]") {
    TempDir tmp;
    auto j = nlohmann::json::parse(read_file(kAc1));
    j["water_level_m"] = 12.0;      // above the crest
    j["friction_angle_deg"] = 95.0; // out of range
    const std::string target = (tmp.path / "result.json").string();
    const Outcome r = run({"-o", target}, j.dump());
    CHECK(r.code == 1);
    CHECK(r.out.empty());
    CHECK(r.err.find("water_level_above_crest") != std::string::npos);
    CHECK(r.err.find("water_level_m") != std::string::npos);
    CHECK(r.err.find("friction_angle_out_of_range") != std::string::npos);
    CHECK_FALSE(fs::exists(target));
}

TEST_CASE("CLI-02 a malformed or mis-shaped document exits 1 with a message", "[FR-18][CLI-02]") {
    const Outcome broken = run({}, "{ not json");
    CHECK(broken.code == 1);
    CHECK(broken.err.find("malformed_json") != std::string::npos);
    CHECK(broken.out.empty());

    const Outcome typo = run({}, R"({"schema_version": 1, "oops": 1})");
    CHECK(typo.code == 1);
    CHECK(typo.err.find("unknown_field") != std::string::npos);
    CHECK(typo.err.find("oops") != std::string::npos);
}

TEST_CASE("CLI-03 an unreadable input file exits 2 with a message", "[UC-07][CLI-03]") {
    const Outcome r = run({"/nonexistent/dir/scenario.json"});
    CHECK(r.code == 2);
    CHECK(r.err.find("/nonexistent/dir/scenario.json") != std::string::npos);
    CHECK(r.out.empty());
}

TEST_CASE("CLI-03 an unwritable output path exits 2 with a message", "[UC-07][CLI-03]") {
    const Outcome r = run({kAc1, "-o", "/nonexistent/dir/result.json"});
    CHECK(r.code == 2);
    CHECK(r.err.find("/nonexistent/dir/result.json") != std::string::npos);
}

TEST_CASE("CLI usage errors exit 2 and --help exits 0", "[FR-18][CLI-03]") {
    CHECK(run({"--bogus"}).code == 2);
    CHECK(run({kAc1, kAc1}).code == 2);
    CHECK(run({"-o"}).code == 2);
    const Outcome help = run({"--help"});
    CHECK(help.code == 0);
    CHECK(help.out.find("Usage") != std::string::npos);
}
