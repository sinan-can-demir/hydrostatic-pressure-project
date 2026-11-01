#include "io/Json.hpp"

#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>

#include <nlohmann/json.hpp>

namespace io {

namespace {

using json = nlohmann::json;
using ojson = nlohmann::ordered_json;

constexpr std::string_view kPresetName = "usace_em_1110_2_2200";

std::string join(const std::string& base, std::string_view key) {
    return base.empty() ? std::string(key) : base + "." + std::string(key);
}

// ---- reading -------------------------------------------------------------------------------

class Reader {
public:
    std::vector<IoError> errors;

    void add(IoErrorCode code, std::string path, std::string message) {
        errors.push_back({code, std::move(path), std::move(message)});
    }

    bool object(const json& value, const std::string& path) {
        if (value.is_object()) return true;
        add(IoErrorCode::WrongType, path, "expected an object");
        return false;
    }

    void reject_unknown(const json& obj, const std::string& path,
                        std::initializer_list<std::string_view> allowed) {
        for (const auto& item : obj.items()) {
            bool known = false;
            for (std::string_view name : allowed) known = known || item.key() == name;
            if (!known) add(IoErrorCode::UnknownField, join(path, item.key()), "unknown field");
        }
    }

    // The member, or null when absent (reporting it if required).
    const json* member(const json& obj, const std::string& path, std::string_view key, bool required) {
        const auto it = obj.find(std::string(key));
        if (it == obj.end()) {
            if (required) add(IoErrorCode::MissingField, join(path, key), "required field is missing");
            return nullptr;
        }
        return &*it;
    }

    std::optional<double> number(const json& value, const std::string& path) {
        if (value.is_number()) return value.get<double>();
        add(IoErrorCode::WrongType, path, "expected a number");
        return std::nullopt;
    }

    std::optional<std::string> string(const json& value, const std::string& path) {
        if (value.is_string()) return value.get<std::string>();
        add(IoErrorCode::WrongType, path, "expected a string");
        return std::nullopt;
    }

    // Reads an optional number member into `target` when present.
    void read_number(const json& obj, const std::string& path, std::string_view key, double& target) {
        if (const json* v = member(obj, path, key, false)) {
            if (auto n = number(*v, join(path, key))) target = *n;
        }
    }
};

template <typename E> struct Name {
    std::string_view text;
    E value;
};

template <typename E, std::size_t N>
void read_enum(Reader& r, const json& value, const std::string& path, const Name<E> (&names)[N], E& target) {
    const auto text = r.string(value, path);
    if (!text) return;
    for (const auto& n : names) {
        if (*text == n.text) {
            target = n.value;
            return;
        }
    }
    std::string choices;
    for (const auto& n : names) choices += (choices.empty() ? "" : ", ") + std::string(n.text);
    r.add(IoErrorCode::InvalidValue, path, "unknown value \"" + *text + "\"; expected one of: " + choices);
}

constexpr Name<model::LoadCondition> kConditions[] = {{"usual", model::LoadCondition::Usual},
                                                      {"unusual", model::LoadCondition::Unusual},
                                                      {"extreme", model::LoadCondition::Extreme}};
constexpr Name<model::ResultantZone> kZones[] = {{"middle_third", model::ResultantZone::MiddleThird},
                                                 {"middle_half", model::ResultantZone::MiddleHalf},
                                                 {"within_base", model::ResultantZone::WithinBase}};

void read_row(Reader& r, const json& value, const std::string& path, model::CriteriaRow& row) {
    if (!r.object(value, path)) return;
    r.reject_unknown(value, path, {"zone", "min_sliding_fs"});
    if (const json* z = r.member(value, path, "zone", false))
        read_enum(r, *z, join(path, "zone"), kZones, row.zone);
    r.read_number(value, path, "min_sliding_fs", row.min_sliding_fs);
}

void read_criteria(Reader& r, const json& value, model::CriteriaSet& criteria) {
    const std::string path = "criteria";
    if (!r.object(value, path)) return;
    r.reject_unknown(value, path, {"preset", "usual", "unusual", "extreme"});
    if (const json* p = r.member(value, path, "preset", false)) {
        if (auto name = r.string(*p, "criteria.preset"); name && *name != kPresetName) {
            r.add(IoErrorCode::InvalidValue, "criteria.preset",
                  "unknown preset \"" + *name + "\"; expected \"" + std::string(kPresetName) + "\"");
        }
    }
    // Rows replace the preset values field by field.
    if (const json* v = r.member(value, path, "usual", false))
        read_row(r, *v, "criteria.usual", criteria.usual);
    if (const json* v = r.member(value, path, "unusual", false))
        read_row(r, *v, "criteria.unusual", criteria.unusual);
    if (const json* v = r.member(value, path, "extreme", false))
        read_row(r, *v, "criteria.extreme", criteria.extreme);
}

void read_vertices(Reader& r, const json& dam, std::vector<math::Point2>& out) {
    const std::string path = "dam.vertices_m";
    const json* list = r.member(dam, "dam", "vertices_m", true);
    if (list == nullptr) return;
    if (!list->is_array()) {
        r.add(IoErrorCode::WrongType, path, "expected an array of [x, y] pairs");
        return;
    }
    for (std::size_t i = 0; i < list->size(); ++i) {
        const json& pair = (*list)[i];
        const std::string vertex_path = path + "[" + std::to_string(i) + "]";
        if (!pair.is_array() || pair.size() != 2) {
            r.add(IoErrorCode::WrongType, vertex_path, "expected a pair [x, y]");
            continue;
        }
        const auto x = r.number(pair[0], vertex_path + "[0]");
        const auto y = r.number(pair[1], vertex_path + "[1]");
        if (x && y) out.push_back({units::Length(*x), units::Length(*y)});
    }
}

// ---- writing -------------------------------------------------------------------------------

// Degrees whose conversion gives back exactly this angle, so an echoed input reproduces the
// run bit for bit. Plain radians -> degrees can be one ulp off the number the user typed.
double degrees_for(units::Angle angle) {
    const double plain = angle.degrees();
    if (units::Angle::from_degrees(plain) == angle) return plain;
    double up = plain;
    double down = plain;
    for (int step = 0; step < 4; ++step) {
        up = std::nextafter(up, std::numeric_limits<double>::infinity());
        if (units::Angle::from_degrees(up) == angle) return up;
        down = std::nextafter(down, -std::numeric_limits<double>::infinity());
        if (units::Angle::from_degrees(down) == angle) return down;
    }
    return plain;
}

std::string_view name_of(model::LoadCondition c) {
    for (const auto& n : kConditions)
        if (n.value == c) return n.text;
    return "usual";
}
std::string_view name_of(model::ResultantZone z) {
    for (const auto& n : kZones)
        if (n.value == z) return n.text;
    return "middle_third";
}
std::string_view name_of(simulation::ContactState s) {
    switch (s) {
    case simulation::ContactState::Full:
        return "full";
    case simulation::ContactState::Partial:
        return "partial";
    case simulation::ContactState::None:
        return "none";
    }
    return "none";
}
std::string_view name_of(simulation::MomentRole r) {
    switch (r) {
    case simulation::MomentRole::Resisting:
        return "resisting";
    case simulation::MomentRole::Overturning:
        return "overturning";
    case simulation::MomentRole::Neutral:
        return "neutral";
    }
    return "neutral";
}
std::string_view name_of(simulation::ForceDirection d) {
    return d == simulation::ForceDirection::Horizontal ? "horizontal" : "vertical";
}
std::string_view name_of(simulation::CheckStatus s) {
    switch (s) {
    case simulation::CheckStatus::Pass:
        return "pass";
    case simulation::CheckStatus::Fail:
        return "fail";
    case simulation::CheckStatus::Skipped:
        return "skipped";
    }
    return "skipped";
}

ojson optional_number(const std::optional<double>& v) { return v ? ojson(*v) : ojson(nullptr); }

ojson row_to_json(const model::CriteriaRow& row) {
    return {{"zone", name_of(row.zone)}, {"min_sliding_fs", row.min_sliding_fs}};
}

ojson scenario_value(const model::Scenario& s) {
    ojson vertices = ojson::array();
    for (const math::Point2& p : s.section.vertices) vertices.push_back({p.x.meters, p.y.meters});

    ojson j;
    j["schema_version"] = kSchemaVersion;
    j["dam"] = {{"vertices_m", vertices}};
    j["water_level_m"] = s.water_level.elevation.meters;
    j["friction_angle_deg"] = degrees_for(s.foundation.friction_angle);
    j["load_condition"] = name_of(s.condition);
    j["fluid"] = {{"density_kg_m3", s.fluid.density.kg_per_m3}, {"gravity_m_s2", s.fluid.gravity.m_per_s2}};
    j["concrete"] = {{"unit_weight_n_m3", s.concrete.unit_weight.n_per_m3}};
    j["allowable_bearing_pa"] =
        s.foundation.allowable_bearing ? ojson(s.foundation.allowable_bearing->pascals) : ojson(nullptr);
    j["criteria"] = {{"usual", row_to_json(s.criteria.usual)},
                     {"unusual", row_to_json(s.criteria.unusual)},
                     {"extreme", row_to_json(s.criteria.extreme)}};
    j["pressure_samples"] = s.pressure_samples ? ojson(*s.pressure_samples) : ojson(nullptr);
    return j;
}

} // namespace

std::expected<model::Scenario, std::vector<IoError>> parse_scenario(std::string_view text) {
    json root;
    try {
        root = json::parse(text.begin(), text.end());
    } catch (const json::parse_error& e) {
        return std::unexpected(std::vector<IoError>{{IoErrorCode::MalformedJson, "", e.what()}});
    }

    Reader r;
    if (!r.object(root, "")) return std::unexpected(std::move(r.errors));

    // The version decides how to read everything else, so it is settled first.
    const json* version = r.member(root, "", "schema_version", true);
    if (version == nullptr) return std::unexpected(std::move(r.errors));
    if (!version->is_number_integer()) {
        r.add(IoErrorCode::WrongType, "schema_version", "expected an integer");
        return std::unexpected(std::move(r.errors));
    }
    if (version->get<long long>() != kSchemaVersion) {
        r.add(IoErrorCode::UnsupportedSchemaVersion, "schema_version",
              "schema_version " + std::to_string(version->get<long long>()) +
                  " is not supported; this tool reads version " + std::to_string(kSchemaVersion));
        return std::unexpected(std::move(r.errors));
    }

    r.reject_unknown(root, "",
                     {"schema_version", "dam", "water_level_m", "friction_angle_deg", "load_condition",
                      "fluid", "concrete", "allowable_bearing_pa", "criteria", "pressure_samples"});

    model::DamSection section;
    if (const json* dam = r.member(root, "", "dam", true); dam != nullptr && r.object(*dam, "dam")) {
        r.reject_unknown(*dam, "dam", {"vertices_m"});
        read_vertices(r, *dam, section.vertices);
    }

    double water_level = 0.0;
    if (const json* v = r.member(root, "", "water_level_m", true)) {
        if (auto n = r.number(*v, "water_level_m")) water_level = *n;
    }
    double friction_deg = 0.0;
    if (const json* v = r.member(root, "", "friction_angle_deg", true)) {
        if (auto n = r.number(*v, "friction_angle_deg")) friction_deg = *n;
    }

    model::LoadCondition condition = model::LoadCondition::Usual;
    if (const json* v = r.member(root, "", "load_condition", false)) {
        read_enum(r, *v, "load_condition", kConditions, condition);
    }

    materials::Fluid fluid;
    if (const json* v = r.member(root, "", "fluid", false); v != nullptr && r.object(*v, "fluid")) {
        r.reject_unknown(*v, "fluid", {"density_kg_m3", "gravity_m_s2"});
        r.read_number(*v, "fluid", "density_kg_m3", fluid.density.kg_per_m3);
        r.read_number(*v, "fluid", "gravity_m_s2", fluid.gravity.m_per_s2);
    }

    materials::Concrete concrete;
    if (const json* v = r.member(root, "", "concrete", false); v != nullptr && r.object(*v, "concrete")) {
        r.reject_unknown(*v, "concrete", {"unit_weight_n_m3"});
        r.read_number(*v, "concrete", "unit_weight_n_m3", concrete.unit_weight.n_per_m3);
    }

    std::optional<units::Pressure> bearing;
    if (const json* v = r.member(root, "", "allowable_bearing_pa", false); v != nullptr && !v->is_null()) {
        if (auto n = r.number(*v, "allowable_bearing_pa")) bearing = units::Pressure(*n);
    }

    model::CriteriaSet criteria = model::CriteriaSet::usace();
    if (const json* v = r.member(root, "", "criteria", false)) read_criteria(r, *v, criteria);

    std::optional<std::size_t> samples;
    if (const json* v = r.member(root, "", "pressure_samples", false); v != nullptr && !v->is_null()) {
        if (v->is_number_unsigned()) {
            samples = v->get<std::size_t>();
        } else {
            r.add(IoErrorCode::WrongType, "pressure_samples", "expected a non-negative integer");
        }
    }

    if (!r.errors.empty()) return std::unexpected(std::move(r.errors));

    model::Scenario scenario{.section = std::move(section),
                             .water_level = model::WaterLevel{units::Length(water_level)},
                             .foundation =
                                 model::Foundation{units::Angle::from_degrees(friction_deg), bearing},
                             .condition = condition,
                             .fluid = fluid,
                             .concrete = concrete,
                             .criteria = criteria,
                             .pressure_samples = samples};
    return scenario;
}

std::string scenario_to_json(const model::Scenario& scenario) {
    return scenario_value(scenario).dump(2) + "\n";
}

std::string result_to_json(const simulation::Result& r) {
    using simulation::Result;

    ojson j;
    j["schema_version"] = kSchemaVersion;
    j["input"] = scenario_value(r.scenario);

    ojson pressure = ojson::array();
    for (const simulation::PressureSample& s : r.pressure.samples) {
        pressure.push_back({{"x_m", s.position.x.meters},
                            {"y_m", s.position.y.meters},
                            {"pressure_pa", s.pressure.pascals},
                            {"normal", {s.normal.x.meters, s.normal.y.meters}}});
    }
    j["pressure"] = pressure;

    ojson contributions = ojson::array();
    for (const simulation::ForceContribution& c : r.contributions) {
        contributions.push_back({{"source", c.source},
                                 {"fx_n_m", c.fx.n_per_m},
                                 {"fy_n_m", c.fy.n_per_m},
                                 {"x_m", c.point.x.meters},
                                 {"y_m", c.point.y.meters}});
    }
    j["contributions"] = contributions;

    const simulation::Resultant& res = r.resultant;
    j["resultant"] = {{"vertical_n_m", res.vertical.n_per_m},
                      {"horizontal_n_m", res.horizontal.n_per_m},
                      {"moment_about_toe_n_m_m", res.moment_about_toe.n_m_per_m},
                      {"x_from_toe_m", res.x_from_toe ? ojson(res.x_from_toe->meters) : ojson(nullptr)},
                      {"eccentricity_m", res.eccentricity ? ojson(res.eccentricity->meters) : ojson(nullptr)},
                      {"base_length_m", res.base_length.meters}};

    j["contact"] = {{"state", name_of(r.contact.state)},
                    {"length_m", r.contact.length.meters},
                    {"toe_pressure_pa", r.contact.toe_pressure.pascals},
                    {"heel_pressure_pa", r.contact.heel_pressure.pascals}};

    j["sliding"] = {{"factor_of_safety", optional_number(r.sliding.factor_of_safety)},
                    {"no_driving_shear", !r.sliding.factor_of_safety.has_value()}};

    ojson terms = ojson::array();
    for (const simulation::OverturningTerm& t : r.overturning.terms) {
        terms.push_back({{"source", t.source},
                         {"direction", name_of(t.direction)},
                         {"force_n_m", t.force.n_per_m},
                         {"arm_m", t.arm.meters},
                         {"moment_n_m_m", t.moment.n_m_per_m},
                         {"role", name_of(t.role)}});
    }
    j["overturning"] = {{"terms", terms},
                        {"resisting_moment_n_m_m", r.overturning.resisting.n_m_per_m},
                        {"overturning_moment_n_m_m", r.overturning.overturning.n_m_per_m},
                        {"ratio", optional_number(r.overturning.ratio)},
                        {"unbounded", !r.overturning.ratio.has_value()}};

    const simulation::Checks& c = r.checks;
    j["checks"] = {
        {"location",
         {{"status", name_of(c.location.status)},
          {"zone", name_of(c.location.zone)},
          {"lower_m", c.location.lower.meters},
          {"upper_m", c.location.upper.meters}}},
        {"sliding",
         {{"status", name_of(c.sliding.status)},
          {"factor_of_safety", optional_number(c.sliding.factor_of_safety)},
          {"minimum", c.sliding.minimum}}},
        {"bearing",
         {{"status", name_of(c.bearing.status)},
          {"max_pressure_pa", c.bearing.max_pressure.pascals},
          {"allowable_pa", c.bearing.allowable ? ojson(c.bearing.allowable->pascals) : ojson(nullptr)}}}};

    j["warnings"] = r.warnings;
    j["verdict"] = r.verdict == simulation::Verdict::Pass ? "pass" : "fail";
    return j.dump(2) + "\n";
}

} // namespace io
