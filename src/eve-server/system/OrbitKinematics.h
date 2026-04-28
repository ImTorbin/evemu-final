/*
 * Documented orbit radius / angular-rate helper for DestinyManager NPC orbit paths.
 */
#ifndef EVEMU_ORBIT_KINEMATICS_H_
#define EVEMU_ORBIT_KINEMATICS_H_

namespace OrbitKinematics {

struct Params {
    double cmd_m {};              ///< commanded standoff (NPC AI orbit command)
    double self_radius_m {};
    double target_radius_m {};
    double max_ship_speed_mps {};
    double ship_agility {};       ///< s/Mkg — used only by legacy-compatible radius solver
};

struct Result {
    double r_follow_m {};         ///< center-to-center ring radius encoded as m_followDistance / CmdOrbit
    double omega_rad_s {};
    double tangential_speed_mps {};
    double orbit_period_s {};
};

/** Compute orbital parameters for orbiting SE::Orbit(SE, cmd). Matches legacy clamps; replaces inline Scheulagh block. */
Result ComputeLegacyCompatible(const Params& p);

/** Simple circular kinematics when agility model is irrelevant (spread tests / asserts). */
Result ComputeCircular(const Params& p, double radial_scale = 1.0);

}

#endif

