#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "io/IoError.hpp"
#include "io/Json.hpp"
#include "model/Scenario.hpp"
#include "simulation/Analyzer.hpp"
#include "units/Angle.hpp"

using Catch::Matchers::WithinRel;
using namespace units;
using math::Point2;
using model::DamSection;
using model::Scenario;

namespace {

const std::string kDataDir = HP_TEST_DATA_DIR;

std::string read_file(const std::string& name) {
    std::ifstream in(kDataDir + "/" + name, std::ios::binary);
    REQUIRE(in.good());
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

Point2 P(double x, double y) { return Point2{Length(x), Length(y)}; }

Scenario ac1() {
    return Scenario{.section = DamSection{{P(0, 0), P(5, 0), P(5, 10), P(0, 10)}},
                    .water_level = model::WaterLevel{Length(10.0)},
                    .foundation = model::Foundation{Angle::from_degrees(35.0)}};
}

// A minimal valid document with one top-level member replaced, added or (null replacement
// meaning "remove") dropped.
std::string with(const std::string& key, const nlohmann::json& value) {
    nlohmann::json j = nlohmann::json::parse(read_file("ac1_scenario.json"));
    if (value.is_discarded()) {
        j.erase(key);
    } else {
        j[key] = value;
    }
    return j.dump();
}
const nlohmann::json kRemove = nlohmann::json::value_t::discarded;

bool has(const std::vector<io::IoError>& errors, io::IoErrorCode code, const std::string& path) {
    for (const auto& e : errors) {
        if (e.code == code && e.path == path) return true;
    }
    return false;
}

} // namespace

// ---- I-01 parsing ---------------------------------------------------------------------------

TEST_CASE("I-01 the AC-1 document parses to the expected Scenario", "[FR-17][FR-03][I-01]") {
    const auto parsed = io::parse_scenario(read_file("ac1_scenario.json"));
    REQUIRE(parsed.has_value());
    CHECK(*parsed == ac1());
}

TEST_CASE("I-01 only the required fields are needed; defaults fill the rest", "[FR-03][I-01]") {
    const auto parsed = io::parse_scenario(R"({
        "schema_version": 1,
        "dam": {"vertices_m": [[0,0],[5,0],[5,10],[0,10]]},
        "water_level_m": 10,
        "friction_angle_deg": 35
    })");
    REQUIRE(parsed.has_value());
    CHECK(*parsed == ac1());
}

TEST_CASE("I-01 optional fields are read", "[FR-03][FR-13][FR-14][FR-20][I-01]") {
    const auto parsed = io::parse_scenario(R"({
        "schema_version": 1,
        "dam": {"vertices_m": [[0,0],[5,0],[5,10],[0,10]]},
        "water_level_m": 6,
        "friction_angle_deg": 30,
        "load_condition": "extreme",
        "fluid": {"density_kg_m3": 1025},
        "concrete": {"unit_weight_n_m3": 24000},
        "allowable_bearing_pa": 500000,
        "criteria": {"preset": "usace_em_1110_2_2200",
                     "usual": {"min_sliding_fs": 1.5, "zone": "middle_half"}},
        "pressure_samples": 7
    })");
    REQUIRE(parsed.has_value());
    CHECK(parsed->water_level.elevation.meters == 6.0);
    CHECK(parsed->condition == model::LoadCondition::Extreme);
    CHECK(parsed->fluid.density.kg_per_m3 == 1025.0);
    CHECK(parsed->fluid.gravity.m_per_s2 == 9.81);
    CHECK(parsed->concrete.unit_weight.n_per_m3 == 24000.0);
    REQUIRE(parsed->foundation.allowable_bearing.has_value());
    CHECK(parsed->foundation.allowable_bearing->pascals == 500000.0);
    CHECK(parsed->criteria.usual.min_sliding_fs == 1.5);
    CHECK(parsed->criteria.usual.zone == model::ResultantZone::MiddleHalf);
    CHECK(parsed->criteria.unusual == model::CriteriaSet::usace().unusual);
    CHECK(parsed->pressure_samples == 7);
}

// ---- I-02 errors ----------------------------------------------------------------------------

TEST_CASE("I-02 malformed JSON gives a clear error", "[FR-17][I-02]") {
    const auto r = io::parse_scenario("{ \"schema_version\": 1, ");
    REQUIRE_FALSE(r.has_value());
    REQUIRE(r.error().size() == 1);
    CHECK(r.error()[0].code == io::IoErrorCode::MalformedJson);
    CHECK_FALSE(r.error()[0].message.empty());
}

TEST_CASE("I-02 the top level must be an object", "[FR-17][I-02]") {
    const auto r = io::parse_scenario("[1, 2]");
    REQUIRE_FALSE(r.has_value());
    CHECK(has(r.error(), io::IoErrorCode::WrongType, ""));
}

TEST_CASE("I-02 schema_version is required and must be supported", "[FR-19][I-02]") {
    auto missing = io::parse_scenario(with("schema_version", kRemove));
    REQUIRE_FALSE(missing.has_value());
    CHECK(has(missing.error(), io::IoErrorCode::MissingField, "schema_version"));

    auto future = io::parse_scenario(with("schema_version", 2));
    REQUIRE_FALSE(future.has_value());
    REQUIRE(future.error().size() == 1);
    CHECK(future.error()[0].code == io::IoErrorCode::UnsupportedSchemaVersion);
    CHECK(future.error()[0].message.find("2") != std::string::npos);

    auto wrong = io::parse_scenario(with("schema_version", "1"));
    CHECK(has(wrong.error(), io::IoErrorCode::WrongType, "schema_version"));
}

TEST_CASE("I-02 missing required fields are named", "[FR-17][I-02]") {
    for (const char* key : {"dam", "water_level_m", "friction_angle_deg"}) {
        const auto r = io::parse_scenario(with(key, kRemove));
        REQUIRE_FALSE(r.has_value());
        CHECK(has(r.error(), io::IoErrorCode::MissingField, key));
    }
}

TEST_CASE("I-02 unknown keys are rejected at every level", "[FR-17][I-02]") {
    CHECK(has(io::parse_scenario(with("water_levl_m", 3)).error(), io::IoErrorCode::UnknownField,
              "water_levl_m"));
    CHECK(has(io::parse_scenario(with("fluid", nlohmann::json{{"densty", 1}})).error(),
              io::IoErrorCode::UnknownField, "fluid.densty"));
    CHECK(has(io::parse_scenario(with("dam", nlohmann::json{{"vertices_m", nlohmann::json::array()},
                                                            {"extra", 1}})).error(),
              io::IoErrorCode::UnknownField, "dam.extra"));
    CHECK(has(io::parse_scenario(with("criteria", nlohmann::json{{"usual", {{"zon", "x"}}}})).error(),
              io::IoErrorCode::UnknownField, "criteria.usual.zon"));
}

TEST_CASE("I-02 wrong types are rejected with the offending path", "[FR-17][I-02]") {
    CHECK(has(io::parse_scenario(with("water_level_m", "10")).error(), io::IoErrorCode::WrongType,
              "water_level_m"));
    CHECK(has(io::parse_scenario(with("fluid", 5)).error(), io::IoErrorCode::WrongType, "fluid"));
    CHECK(has(io::parse_scenario(with("dam", nlohmann::json{{"vertices_m", "x"}})).error(),
              io::IoErrorCode::WrongType, "dam.vertices_m"));
    CHECK(has(io::parse_scenario(with("dam", nlohmann::json{{"vertices_m", {{0, 0}, {5}, {5, 10}}}})).error(),
              io::IoErrorCode::WrongType, "dam.vertices_m[1]"));
    CHECK(has(io::parse_scenario(with("dam", nlohmann::json{{"vertices_m", {{0, 0}, {"a", 0}, {5, 10}}}})).error(),
              io::IoErrorCode::WrongType, "dam.vertices_m[1][0]"));
    CHECK(has(io::parse_scenario(with("pressure_samples", 2.5)).error(), io::IoErrorCode::WrongType,
              "pressure_samples"));
    CHECK(has(io::parse_scenario(with("pressure_samples", -3)).error(), io::IoErrorCode::WrongType,
              "pressure_samples"));
}

TEST_CASE("I-02 unknown enumeration values are rejected", "[FR-13][I-02]") {
    CHECK(has(io::parse_scenario(with("load_condition", "rare")).error(), io::IoErrorCode::InvalidValue,
              "load_condition"));
    CHECK(has(io::parse_scenario(with("criteria", nlohmann::json{{"preset", "eurocode"}})).error(),
              io::IoErrorCode::InvalidValue, "criteria.preset"));
    CHECK(has(io::parse_scenario(with("criteria", nlohmann::json{{"usual", {{"zone", "middle_fifth"}}}})).error(),
              io::IoErrorCode::InvalidValue, "criteria.usual.zone"));
}

TEST_CASE("I-02 all problems are reported together", "[FR-17][I-02]") {
    const auto r = io::parse_scenario(R"({
        "schema_version": 1, "dam": 3, "water_level_m": "x", "typo": 1
    })");
    REQUIRE_FALSE(r.has_value());
    CHECK(r.error().size() >= 4);  // dam type, water type, missing friction, unknown key
}

// ---- I-03 golden file, I-04 echo ---------------------------------------------------------

TEST_CASE("I-03 the AC-1 result JSON equals the stored golden file", "[FR-17][I-03]") {
    const auto result = simulation::analyze(ac1());
    REQUIRE(result.has_value());
    const std::string text = io::result_to_json(*result);
    if (std::getenv("HP_UPDATE_GOLDEN") != nullptr) {
        std::ofstream(kDataDir + "/ac1_result.json", std::ios::binary) << text;
    }
    CHECK(text == read_file("ac1_result.json"));
}

TEST_CASE("I-03 the result JSON carries the intermediate values and the warning", "[NFR-09][FR-15][I-03]") {
    const auto result = simulation::analyze(ac1());
    const auto j = nlohmann::json::parse(io::result_to_json(*result));
    CHECK(j["schema_version"] == 1);
    CHECK(j["verdict"] == "fail");
    CHECK(j["warnings"][0] == "uplift not modelled; results are unconservative for real dams");
    CHECK_THAT(j["resultant"]["x_from_toe_m"].get<double>(), WithinRel(1.10851, 1e-5));
    CHECK(j["overturning"]["terms"].size() == 2);
    CHECK(j["contributions"][0]["source"] == "water (horizontal)");
    CHECK(j["checks"]["location"]["status"] == "fail");
    CHECK(j["checks"]["bearing"]["status"] == "skipped");
    CHECK(j["contact"]["state"] == "partial");
}

TEST_CASE("I-03 absent values are null, not omitted", "[FR-10][FR-11][I-03]") {
    Scenario s = ac1();
    s.water_level = model::WaterLevel{Length(0.0)};
    const auto j = nlohmann::json::parse(io::result_to_json(*simulation::analyze(s)));
    CHECK(j["sliding"]["factor_of_safety"].is_null());
    CHECK(j["sliding"]["no_driving_shear"] == true);
    CHECK(j["overturning"]["ratio"].is_null());
    CHECK(j["overturning"]["unbounded"] == true);
}

TEST_CASE("I-04 the echoed input reproduces the result", "[UC-07][NFR-03][I-04]") {
    for (const double phi : {35.0, 0.9, 3.7, 29.3}) {
        Scenario s = ac1();
        s.foundation.friction_angle = Angle::from_degrees(phi);
        s.pressure_samples = 4;
        s.foundation.allowable_bearing = Pressure(750000.0);
        const auto first = simulation::analyze(s);
        REQUIRE(first.has_value());
        const auto j = nlohmann::json::parse(io::result_to_json(*first));
        const auto reparsed = io::parse_scenario(j["input"].dump());
        REQUIRE(reparsed.has_value());
        CHECK(*reparsed == s);
        CHECK(*simulation::analyze(*reparsed) == *first);
    }
}

TEST_CASE("I-04 scenario_to_json round-trips through parse_scenario", "[FR-17][I-04]") {
    Scenario s = ac1();
    s.condition = model::LoadCondition::Unusual;
    s.fluid.density = Density(1025.5);
    s.criteria.extreme.min_sliding_fs = 1.25;
    const auto back = io::parse_scenario(io::scenario_to_json(s));
    REQUIRE(back.has_value());
    CHECK(*back == s);
}

// ---- I-05 -------------------------------------------------------------------------------------

TEST_CASE("I-05 pressure_samples in the document gives that many samples in the output", "[FR-20][I-05]") {
    const auto parsed = io::parse_scenario(with("pressure_samples", 5));
    REQUIRE(parsed.has_value());
    const auto j = nlohmann::json::parse(io::result_to_json(*simulation::analyze(*parsed)));
    REQUIRE(j["pressure"].size() == 5);
    CHECK_THAT(j["pressure"][2]["y_m"].get<double>(), WithinRel(5.0, 1e-12));
}

TEST_CASE("I-02 every IO error code has its documented stable name", "[FR-19][I-02]") {
    // These names appear in CLI output and in docs/07-json-schema.md.
    CHECK(io::to_string(io::IoErrorCode::MalformedJson) == "malformed_json");
    CHECK(io::to_string(io::IoErrorCode::UnsupportedSchemaVersion) == "unsupported_schema_version");
    CHECK(io::to_string(io::IoErrorCode::MissingField) == "missing_field");
    CHECK(io::to_string(io::IoErrorCode::UnknownField) == "unknown_field");
    CHECK(io::to_string(io::IoErrorCode::WrongType) == "wrong_type");
    CHECK(io::to_string(io::IoErrorCode::InvalidValue) == "invalid_value");
}

TEST_CASE("I-02 a string field given a non-string is a type error", "[FR-17][I-02]") {
    CHECK(has(io::parse_scenario(with("load_condition", 5)).error(), io::IoErrorCode::WrongType,
              "load_condition"));
    CHECK(has(io::parse_scenario(with("criteria", nlohmann::json{{"preset", 1}})).error(),
              io::IoErrorCode::WrongType, "criteria.preset"));
}
