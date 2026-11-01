#include "FaceLoad.hpp"

namespace simulation::detail {

namespace {

// Integral over t in [0,1] of q(t) * f(t), where q is linear from q0 to q1 and f is linear
// from f0 to f1: exact for the product of two linear functions.
double linear_product(double q0, double q1, double f0, double f1) {
    const double dq = q1 - q0;
    const double df = f1 - f0;
    return q0 * f0 + (q0 * df + dq * f0) / 2.0 + dq * df / 3.0;
}

} // namespace

std::vector<Pt> wetted_chain(const std::vector<math::Point2>& face, double level) {
    std::vector<Pt> chain;
    for (std::size_t i = 0; i + 1 < face.size(); ++i) {
        const Pt a{face[i].x.meters, face[i].y.meters};
        const Pt b{face[i + 1].x.meters, face[i + 1].y.meters};
        if (a.y >= level) break;
        if (chain.empty()) chain.push_back(a);
        if (b.y <= level) {
            chain.push_back(b);
        } else {
            const double t = (level - a.y) / (b.y - a.y);
            chain.push_back({a.x + t * (b.x - a.x), level});
            break;
        }
    }
    return chain;
}

std::vector<ForceContribution> face_contributions(const std::vector<Pt>& chain, double level, double k,
                                                  Side side, const char* horizontal_name,
                                                  const char* vertical_name) {
    // Per segment, with q the linear pressure and the force element q * (dy, -dx) (upstream
    // face; the downstream face has the opposite direction): sums of q dy and q dx (the forces)
    // and their first moments about x and y.
    double sum_qdy = 0.0, sum_qx_dy = 0.0, sum_qy_dy = 0.0;
    double sum_qdx = 0.0, sum_qx_dx = 0.0, sum_qy_dx = 0.0;
    for (std::size_t i = 0; i + 1 < chain.size(); ++i) {
        const Pt a = chain[i];
        const Pt b = chain[i + 1];
        const double dx = b.x - a.x;
        const double dy = b.y - a.y;
        const double q0 = k * (level - a.y);
        const double q1 = k * (level - b.y);
        sum_qdy += dy * linear_product(q0, q1, 1.0, 1.0);
        sum_qx_dy += dy * linear_product(q0, q1, a.x, b.x);
        sum_qy_dy += dy * linear_product(q0, q1, a.y, b.y);
        sum_qdx += dx * linear_product(q0, q1, 1.0, 1.0);
        sum_qx_dx += dx * linear_product(q0, q1, a.x, b.x);
        sum_qy_dx += dx * linear_product(q0, q1, a.y, b.y);
    }

    // Upstream: F = (sum q dy, -sum q dx). Downstream: the reverse direction.
    const double sign = side == Side::Upstream ? 1.0 : -1.0;

    std::vector<ForceContribution> result;
    if (sum_qdy != 0.0) {
        result.push_back(ForceContribution{
            units::ForcePerWidth(sign * sum_qdy), units::ForcePerWidth(0.0),
            math::Point2{units::Length(sum_qx_dy / sum_qdy), units::Length(sum_qy_dy / sum_qdy)},
            horizontal_name});
    }
    if (sum_qdx != 0.0) {
        result.push_back(ForceContribution{
            units::ForcePerWidth(0.0), units::ForcePerWidth(-sign * sum_qdx),
            math::Point2{units::Length(sum_qx_dx / sum_qdx), units::Length(sum_qy_dx / sum_qdx)},
            vertical_name});
    }
    return result;
}

} // namespace simulation::detail
