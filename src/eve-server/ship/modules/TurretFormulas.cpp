
 /**
  * @name TurretFormulas.cpp
  *   formulas for turret tracking, to hit, and other specific things
  * @Author:         Allan
  * @date:   10 June 2015
  */

/* default crit chances - can change in server config
 *      NPC  - 1.5%
 *   Player  - 2%
 *   Sentry  - 2%
 *    Drone  - 3%
 *  Concord  - 5%
 */

#include <algorithm>

#include "character/Character.h" // this sets compat includes for this file
#include "npc/NPC.h"
#include "npc/NPCAI.h"
#include "npc/Drone.h"
#include "npc/Sentry.h"
#include "ship/modules/TurretFormulas.h"
#include "ship/modules/TurretModule.h"


/*
 *  CTO-HIT MATH  (the thing that was wrong before)
 *
 *  EVE's tracking formula is:
 *      ChanceToHit = 0.5 ^ ( ((angVel/tracking) * (sigRes/targetSig))^2
 *                          + (max(0, dist - optimalRange) / falloff)^2 )
 *
 *  Where angVel is *angular velocity* of the target relative to the shooter, in rad/s.
 *  Angular velocity is derived from *transversal* velocity:
 *      angVel = transversalSpeed / distance
 *
 *  Transversal speed is the component of the relative velocity that is PERPENDICULAR to
 *  the line-of-sight between shooter and target. A target flying directly toward or away
 *  from the shooter has zero transversal and is trivial to hit regardless of speed.
 *
 *  The previous implementation used `|v_target - v_shooter|` (relative velocity magnitude)
 *  as the "transversal" input. That is wrong: it conflates radial motion with tangential
 *  motion and makes head-on / tail-chase engagements incorrectly penalised. The sentry
 *  variant was even worse, using `|v_target|` (absolute target speed, ignoring the
 *  shooter's frame entirely).
 *
 *  ComputeTransversal() below returns the correct perpendicular component.
 */
namespace {

float ComputeTransversal(const GPoint& shooterPos, const GVector& shooterVel,
                         const GPoint& targetPos,  const GVector& targetVel)
{
    GVector relVel(targetVel - shooterVel);
    GVector los(shooterPos, targetPos);
    const double losLen = los.length();
    if (losLen < 0.001) {
        // Shooter and target coincide; transversal is undefined. Fall back to relative
        // speed so the formula still produces a bounded number rather than NaN/Inf.
        return static_cast<float>(relVel.length());
    }
    const GVector losN = los * (1.0 / losLen);                      // unit LOS vector
    const double  radialSpeed = relVel.dotProduct(losN);            // signed projection onto LOS
    const GVector transVec = relVel - (losN * radialSpeed);         // component perpendicular to LOS
    return static_cast<float>(transVec.length());
}

float GetEffectiveTargetSignature(SystemEntity* pTarget)
{
    // Primary source: explicit signature radius attribute.
    float sig = pTarget->GetSelf()->GetAttribute(AttrSignatureRadius).get_float();
    if (sig >= 1.0f)
        return sig;

    // Fallback: collision radius is much closer to expected ship sig than 0/epsilon.
    // Previous code used radius/10 in one path and epsilon in others, both of which
    // can make hit chance unrealistically low.
    float radius = pTarget->GetSelf()->GetAttribute(AttrRadius).get_float();
    if (radius < 1.0f)
        radius = static_cast<float>(pTarget->GetRadius());

    // Keep a sane lower bound to avoid pathological sigRes/sig blowups.
    if (radius < 25.0f)
        radius = 25.0f;
    return radius;
}

} // namespace

float TurretFormulas::GetToHit(ShipItemRef shipRef, TurretModule* pMod, SystemEntity* pTarget)
{
    if ((pTarget == nullptr) or (pMod == nullptr) or (shipRef.get() == nullptr))
        return 0;
    Client* pPilot = shipRef->GetPilot();
    if ((pPilot == nullptr) or (pPilot->GetShipSE() == nullptr))
        return 0;
    ShipSE* pShipSE = pPilot->GetShipSE();
    if ((pShipSE->DestinyMgr() == nullptr) or (pTarget->DestinyMgr() == nullptr))
        return 0;

    uint32 falloff = pMod->GetAttribute(AttrFalloff).get_uint32();
    const float falloffF = static_cast<float>(std::max(falloff, 1u)); // zero falloff -> inf in (d/falloff)^2
    uint32 range   = pMod->GetAttribute(AttrMaxRange).get_uint32();
    float  distance = pShipSE->GetPosition().distance(pTarget->DestinyMgr()->GetPosition());
    if (distance < 0.001f)
        distance = 0.001f;

    const float transversalV = ComputeTransversal(
        pShipSE->GetPosition(), pShipSE->GetVelocity(),
        pTarget->DestinyMgr()->GetPosition(), pTarget->GetVelocity());
    const float angularVel = transversalV / distance;

    float targSig = GetEffectiveTargetSignature(pTarget);
    float sigRes     = pMod->GetAttribute(AttrOptimalSigRadius).get_float();
    float trackSpeed = pMod->GetAttribute(AttrTrackingSpeed).get_float();
    if (trackSpeed < 0.0001f)
        trackSpeed = 0.0001f;
    if (targSig < 1.0f)
        targSig = 1.0f;
    if (sigRes < 1.0f)
        sigRes = 1.0f;

    _log(DAMAGE__TRACE, "Turret::GetToHit - distance:%.2f, range:%u, falloff:%u", distance, range, falloff);
    _log(DAMAGE__TRACE, "Turret::GetToHit - transversalV:%.3f, angularV:%.5f, tracking:%.3f, targetSig:%.1f, sigRes:%.1f", \
                transversalV, angularVel, trackSpeed, targSig, sigRes);

    /*  calculations for chance to hit  --UD 29May17
     *
     *     a =  angularVel / trackSpeed
     *     b =  sigRes / targSig
     *     c =  (a * b) ^ 2
     *     d =  max(0, distance - optimal range)
     *     e =  (d / falloff) ^ 2
     * tohit =  0.5 ^ (c + e)
     */
    float a = (angularVel / trackSpeed);
    float b = (sigRes / targSig);
    float modifier(0.0f);
    if ((a < 1) and (b > 1)) {
        /* in cases where weapon can track target, but sigRes > targSig, the weapon would not hit on live but *should* hit with reduced damage
         * modify formula to remove Signature variable from equation, test toHit against tracking,
         * then use Signature variables to determine amount of damage reduction (i.e. large gun vs. small ship)
         */
        b = 1;
        modifier = (targSig / sigRes);
    }
    float c = pow((a * b), 2);
    float d = EvE::max(distance - range);
    float e = pow((d / falloffF), 2);
    float x = pow(0.5, c);
    float y = pow(0.5, e);
    float ChanceToHit = x * y;
    _log(DAMAGE__TRACE, "Turret::GetToHit - (%.3f * %.3f)^2 = c:%.5f : (%.3f / %.1f)^2 = e:%.5f", a, b, c, d, falloffF, e);
    float rNum = MakeRandomFloat();
    _log(DAMAGE__TRACE, "Turret::GetToHit - %f * %f = %.5f  - Rand:%.3f  - %s", \
            x, y, ChanceToHit, rNum, ((rNum <= sConfig.rates.PlayerCritChance) ? "Crit" : (rNum < ChanceToHit ? "Hit" : "Miss")));
    if (rNum <= sConfig.rates.PlayerCritChance)
        return 3.0f;
    if (rNum < ChanceToHit) {
        if (modifier)
            return modifier;
        return (rNum + 0.49);
    }
    return 0;
}

float TurretFormulas::GetNPCToHit(NPC* pNPC, SystemEntity* pTarget)
{
    if ((pTarget == nullptr) or (pNPC == nullptr) or (pNPC->GetAIMgr() == nullptr))
        return 0;
    if ((pNPC->DestinyMgr() == nullptr) or (pTarget->DestinyMgr() == nullptr))
        return 0;
    uint16 sigRes  = pNPC->GetAIMgr()->GetSigRes();
    uint16 range   = pNPC->GetAIMgr()->GetOptimalRange();
    uint32 falloff = pNPC->GetAIMgr()->GetFalloff();
    const float falloffF = static_cast<float>(std::max(falloff, 1u)); // Belt NPCs often have AttrFalloff 0 in DB
    float  distance = pNPC->DestinyMgr()->GetPosition().distance(pTarget->DestinyMgr()->GetPosition());
    if (distance < 0.001f)
        distance = 0.001f;
    float trackSpeed = pNPC->GetAIMgr()->GetTrackingSpeed();
    float targSig    = GetEffectiveTargetSignature(pTarget);
    if (trackSpeed < 0.0001f)
        trackSpeed = 0.0001f;
    if (targSig < 1.0f)
        targSig = 1.0f;
    if (sigRes < 1.0f)
        sigRes = 1.0f;

    const float transversalV = ComputeTransversal(
        pNPC->DestinyMgr()->GetPosition(), pNPC->GetVelocity(),
        pTarget->DestinyMgr()->GetPosition(), pTarget->GetVelocity());
    const float angularVel = transversalV / distance;

    _log(DAMAGE__TRACE_NPC, "NPC::GetToHit - distance:%.2f, range:%u, falloff:%u", distance, range, falloff);
    _log(DAMAGE__TRACE_NPC, "NPC::GetToHit - transversalV:%.3f, angularVel:%.5f tracking:%.3f, targetSig:%.1f, sigRes:%u", \
                transversalV, angularVel, trackSpeed, targSig, sigRes);

    float a = (angularVel / trackSpeed);
    float b = (sigRes / targSig);
    float modifier(0.0f);
    if ((a < 1) and (b > 1)) {
        b = 1;
        modifier = (targSig / sigRes);
    }
    float c = pow((a * b), 2);
    float d = EvE::max(distance - range);
    float e = pow((d / falloffF), 2);
    float x = pow(0.5, c);
    float y = pow(0.5, e);
    float ChanceToHit = x * y;
    _log(DAMAGE__TRACE_NPC, "NPC::GetToHit - (%.3f * %.3f)^2 = c:%.5f : (%.3f / %.1f)^2 = e:%.5f", a, b, c, d, falloffF, e);
    _log(DAMAGE__TRACE_NPC, "NPC::GetToHit - %f * %f = %.5f", x, y, ChanceToHit);
    float rNum = MakeRandomFloat(0.0, 1.0);
    _log(DAMAGE__TRACE_NPC, "NPC::GetToHit - ChanceToHit:%f, Rand:%.3f - %s", ChanceToHit, rNum, ((rNum <= sConfig.rates.NpcCritChance) ? "Crit" : (rNum < ChanceToHit ? "Hit" : "Miss")));
    if (rNum <= sConfig.rates.NpcCritChance)
        return 3.0f;
    if (rNum < ChanceToHit) {
        if (modifier)
            return modifier;
        return (rNum + 0.49);
    }
    return 0;
}

float TurretFormulas::GetDroneToHit(DroneSE* pDrone, SystemEntity* pTarget)
{
    InventoryItemRef droneSelf;
    if (pDrone != nullptr)
        droneSelf = pDrone->GetSelf();
    if ((pTarget == nullptr) or (pDrone == nullptr) or (droneSelf.get() == nullptr))
        return 0;
    if ((pDrone->DestinyMgr() == nullptr) or (pTarget->DestinyMgr() == nullptr))
        return 0;

    float falloff = droneSelf->GetAttribute(AttrFalloff).get_float();
    if (falloff < 0.0001f)
        falloff = 0.0001f;
    float distance = pDrone->DestinyMgr()->GetPosition().distance(pTarget->DestinyMgr()->GetPosition());
    if (distance < 0.001f)
        distance = 0.001f;
    float tracking = droneSelf->GetAttribute(AttrTrackingSpeed).get_float();
    if (tracking < 0.0001f)
        tracking = 0.0001f;
    float targetSig = GetEffectiveTargetSignature(pTarget);
    float sigRes = droneSelf->GetAttribute(AttrOptimalSigRadius).get_float();
    if (targetSig < 1.0f)
        targetSig = 1.0f;
    if (sigRes < 1.0f)
        sigRes = 1.0f;

    const float transversalV = ComputeTransversal(
        pDrone->DestinyMgr()->GetPosition(), pDrone->GetVelocity(),
        pTarget->DestinyMgr()->GetPosition(), pTarget->GetVelocity());
    const float angularVel = transversalV / distance;

    // Original drone formula (no sig-mismatch damage-reduction branch -- keep existing behaviour).
    float a = (angularVel / tracking);
    float b = (sigRes / targetSig);
    float c = pow((a * b), 2);
    float d = EvE::max(distance - droneSelf->GetAttribute(AttrEntityAttackRange).get_float());
    float e = pow((d / falloff), 2);
    float ChanceToHit = pow(0.5, c + e);
    _log(DAMAGE__TRACE, "Drone::GetToHit - transversalV:%.3f angularV:%.5f tracking:%.3f dist:%.2f sigRes:%.1f targSig:%.1f chance:%.4f", \
                transversalV, angularVel, tracking, distance, sigRes, targetSig, ChanceToHit);
    float rNum = MakeRandomFloat(0.0, 1.0);
    if (rNum <= sConfig.rates.DroneCritChance)
        return 3.0f;
    if (rNum < ChanceToHit)
        return (rNum + 0.49);
    // drones will have a minimum damage instead of zero
    return 0.1;
}

float TurretFormulas::GetSentryToHit(Sentry* pSentry, SystemEntity* pTarget)
{
    InventoryItemRef sentrySelf;
    if (pSentry != nullptr)
        sentrySelf = pSentry->GetSelf();
    if ((pTarget == nullptr) or (pSentry == nullptr) or (sentrySelf.get() == nullptr))
        return 0;
    if (pTarget->DestinyMgr() == nullptr)
        return 0;
    float sigRes  = sentrySelf->GetAttribute(AttrOptimalSigRadius).get_float();
    float falloff = sentrySelf->GetAttribute(AttrFalloff).get_float();
    if (falloff < 0.0001f)
        falloff = 0.0001f;
    float distance = pSentry->GetPosition().distance(pTarget->DestinyMgr()->GetPosition());
    if (distance < 0.001f)
        distance = 0.001f;
    float tracking = sentrySelf->GetAttribute(AttrTrackingSpeed).get_float();
    if (tracking < 0.0001f)
        tracking = 0.0001f;
    float targSig = GetEffectiveTargetSignature(pTarget);
    if (targSig < 1.0f)
        targSig = 1.0f;
    if (sigRes < 1.0f)
        sigRes = 1.0f;

    // Sentries don't move, but compute transversal properly anyway so the formula is
    // framework-correct and consistent with the other shooter types (and future-proof
    // for any movable deployable variants).
    const float transversalV = ComputeTransversal(
        pSentry->GetPosition(), pSentry->GetVelocity(),
        pTarget->DestinyMgr()->GetPosition(), pTarget->GetVelocity());
    const float angularVel = transversalV / distance;

    float a = (angularVel / tracking);
    float b = (sigRes / targSig);
    float modifier = 0.0f;
    if ((a < 1) and (b > 1)) {
        b = 1;
        modifier = (targSig / sigRes);
    }
    float c = pow((a * b), 2);
    float d = EvE::max(distance - sentrySelf->GetAttribute(AttrEntityAttackRange).get_float());
    float e = pow((d / falloff), 2);
    float ChanceToHit = pow(0.5, c + e);
    _log(DAMAGE__TRACE, "Sentry::GetToHit - transversalV:%.3f angularV:%.5f tracking:%.3f dist:%.2f sigRes:%.1f targSig:%.1f chance:%.4f", \
                transversalV, angularVel, tracking, distance, sigRes, targSig, ChanceToHit);
    float rNum = MakeRandomFloat(0.0, 1.0);
    if (rNum <= sConfig.rates.SentryCritChance)
        return 3.0f;
    if (rNum < ChanceToHit) {
        if (modifier)
            return modifier;
        return (rNum + 0.49);
    }
    return 0;
}
