/*
 * Orbital kinematics helpers for DestinyManager orbiting NPCs/rats/drones.
 */
#include "eve-server.h"

#include "system/OrbitKinematics.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kEdgeBuffer_m = 150.0;

}

namespace OrbitKinematics {

Result ComputeLegacyCompatible(const Params& p)
{
    const double cmd_m = std::max(10.0, p.cmd_m);
    const double m_radius = std::max(1.0, p.self_radius_m);
    const double Tr = std::max(0.0, p.target_radius_m);
    const double Vm = std::max(1e-6, p.max_ship_speed_mps);

    /** Same Rc scaffolding as DestinyManager::Orbit(SE) before follow-radius solve. */
    const double kRcScale = 1.2;
    const double Rc = ((cmd_m + kEdgeBuffer_m + m_radius - (Tr / 12.0)) * kRcScale);

    /** Scheulagh-style closed-form follow radius — kept verbatim from legacy DestinyManager orbit init. */
    const double Vm2 = Vm * Vm;
    const double t2 = std::pow(p.ship_agility > 1e-9 ? p.ship_agility : 1.0, 2);
    const double Rc2 = std::pow(Rc, 2);
    double one = (108 * t2 * Vm2 * Rc2);
    double two = (12 * t2 * Vm2 * std::pow(Rc, 10));
    double three = (12 * std::sqrt(81 * std::pow(std::max(p.ship_agility, 1e-9), 4) * std::pow(Vm, 4) + two));
    double four = (6 * std::cbrt(one + 8 * std::pow(Rc, 6) + three));
    double five = std::cbrt(std::sqrt(three * std::pow(Rc, 8) + two));
    double six = (one + (8 * Rc2) + (12 * five));
    double followD = std::sqrt(four + (24 * std::pow(Rc, 4) / six) + 12 * Rc2) / 6;

    Result r{};
    if (!std::isfinite(followD) || followD <= 0.0) {
        followD = cmd_m + m_radius + Tr;
    }

    /** Fighter / extreme inertia clamps — mirrors DestinyManager.h historic limits. */
    const double minF = std::max(100.0, cmd_m * 0.2);
    const double maxF = std::max(cmd_m * 10.0 + (m_radius + Tr) * 2.0, 5.0e6);
    if (followD < minF || followD > maxF)
        followD = std::max(minF, std::min(followD, maxF));

    followD = std::max(50.0, std::min(followD, 1e8));
    r.r_follow_m = followD;

    /** Tangential speed matching legacy orbit init: Vm * ((cmd/Rf) + 0.065). */
    const double k_lin_blend = (cmd_m / std::max(r.r_follow_m, 1.0)) + 0.065;
    double v = Vm * std::min(10.0, std::max(0.0, k_lin_blend));
    v = std::min(v, Vm * 1.5); // match maxOrbitspeed behaviour

    r.tangential_speed_mps = v;
    r.omega_rad_s = v / std::max(r.r_follow_m, 1.0);
    constexpr double PI2 = 6.28318530717958647692;
    r.orbit_period_s = (r.omega_rad_s > 1e-18) ? PI2 / r.omega_rad_s : 86400.0;
    return r;
}

Result ComputeCircular(const Params& p, double radial_scale)
{
    Result r{};
    const double Vm = std::max(1e-6, p.max_ship_speed_mps);
    const double rnom = std::max(50.0, (std::max(10.0, p.cmd_m) + std::max(1.0, p.self_radius_m) +
        std::max(0.0, p.target_radius_m)) *
        radial_scale);
    r.r_follow_m = rnom;
    double v = std::min(Vm, Vm * 0.75);
    r.tangential_speed_mps = v;
    r.omega_rad_s = v / rnom;
    constexpr double PI2 = 6.28318530717958647692;
    r.orbit_period_s = PI2 / std::max(r.omega_rad_s, 1e-12);
    return r;
}

}
