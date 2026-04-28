/**
 * DroneAI.cpp
 *      this class is for drone AI
 *
 * @Author:     Allan
 * @Version:    0.15
 * @Date:       27Nov19
*/

#include "eve-server.h"

#include "Client.h"
#include "inventory/AttributeEnum.h"
#include "ship/Ship.h"
#include "system/DestinyManager.h"
#include "system/Asteroid.h"
#include "npc/Drone.h"
#include "npc/DroneAI.h"
#include "system/Damage.h"
#include "system/SystemBubble.h"

DroneAIMgr::DroneAIMgr(DroneSE* who)
: m_state(DroneAI::State::Idle),
  m_pDrone(who),
  m_assignedShip(nullptr),
  m_mainAttackTimer(0),// dont start timer until we have a target
  m_processTimer(0),
  m_beginFindTarget(0),
  m_warpScramblerTimer(0),     //not implemented yet
  m_webifierTimer(0),             //not implemented yet
  m_sigRadius(who->GetSelf()->GetAttribute(AttrSignatureRadius).get_float()),
  m_attackSpeed(who->GetSelf()->GetAttribute(AttrSpeed).get_float()),
  m_cruiseSpeed(who->GetSelf()->GetAttribute(AttrEntityCruiseSpeed).get_int()),
  m_chaseSpeed(who->GetSelf()->GetAttribute(AttrMaxVelocity).get_int()),
  m_entityFlyRange(who->GetSelf()->GetAttribute(AttrEntityFlyRange).get_float() + who->GetSelf()->GetAttribute(AttrMaxRange).get_float()),
  m_entityChaseRange(who->GetSelf()->GetAttribute(AttrEntityChaseMaxDistance).get_float() *2),
  m_entityOrbitRange(who->GetSelf()->GetAttribute(AttrMaxRange).get_float()),
  m_entityAttackRange(who->GetSelf()->GetAttribute(AttrEntityAttackRange).get_float() *2),
  m_shieldBoosterDuration(who->GetSelf()->GetAttribute(AttrEntityShieldBoostDuration).get_int()),
  m_armorRepairDuration(who->GetSelf()->GetAttribute(AttrEntityArmorRepairDuration).get_int()),
  m_miningRepeated(false)
{
    m_processTimer.Start(5000);     //arbitrary.

    // proximityRange (154) tells us how far we "see"

    if (m_entityAttackRange < 10000)   // most of these are low...under 6k  that sux for targeting
        m_entityAttackRange *= 3;
    if (m_entityChaseRange < m_entityAttackRange * 2.0)
        m_entityChaseRange = m_entityAttackRange * 2.0;
    if (m_entityChaseRange < 10000.0)
        m_entityChaseRange = 10000.0;
}

uint32 DroneAIMgr::PackedOrbitDistance(double baseMeters) const {
    const double b = EvE::max(500.0, baseMeters);
    const double spread = static_cast<double>((m_pDrone->GetID() % 13u) * 25u);
    double total = b + spread;
    if (total > 200000.0)
        total = 200000.0;
    return static_cast<uint32>(total);
}

void DroneAIMgr::Process() {
    double profileStartTime(GetTimeUSeconds());

    /* Drone::State definitions   -allan 27Nov19
     *   Invalid
     *   Idle              = 0,  // not doing anything....idle.
     *   Combat            = 1,  // fighting - needs targetID
     *   Mining            = 2,  // unsure - needs targetID
     *   Approaching       = 3,  // too close to chase, but to far to engage
     *   Departing         = 4,  // return to ship
     *   Departing2        = 5,  // leaving.  different from Departing
     *   Pursuit           = 6,  // target out of range to attack/follow, but within npc sight range....use mwd/ab if equiped
     *   Fleeing           = 7,  // running away
     *   Operating         = 9,  // whats diff from engaged here?  mining maybe?
     *   Engaged           = 10, // non-combat? - needs targetID
     *   // internal only
     *   Unknown           = 8,  // as stated
     *   Guarding          = 11,
     *   Assisting         = 12,
     *   Incapacitated     = 13  // out of control range, but online
     */

    // test for drone attributes here - aggressive, focus fire, attack/follow
    // test for control distance here also.  offline drones outside this  (AttrDroneControlDistance)
    switch(m_state) {
        case DroneAI::State::Invalid: {
            // check everything in this state.   return to ship?
        } break;
        case DroneAI::State::Idle: {
            // orbiting controlling ship
        } break;
        case DroneAI::State::Engaged: {
            //NOTE: getting our pTarget like this is pretty weak...
            SystemEntity* pTarget = m_pDrone->TargetMgr()->GetFirstTarget(true);
            if (pTarget == nullptr) {
                if (m_pDrone->TargetMgr()->HasNoTargets()) {
                    _log(DRONE__AI_TRACE, "Drone %s(%u): Stopped engagement, GetFirstTarget() returned NULL.", m_pDrone->GetName(), m_pDrone->GetID());
                    SetIdle();
                }
                return;
            } else if (pTarget->SysBubble() == nullptr) {
                m_pDrone->TargetMgr()->ClearTarget(pTarget);
                //m_pDrone->TargetMgr()->OnTarget(pTarget, TargMgr::Mode::Lost);
                return;
            }
            CheckDistance(pTarget);
        } break;
        case DroneAI::State::Mining: {
            SystemEntity* pTarget = m_pDrone->TargetMgr()->GetFirstTarget(true);
            if (pTarget == nullptr) {
                SetIdle();
                return;
            }
            Mine(pTarget);
        } break;

        case DroneAI::State::Departing: { // return to ship.  when close enough, set lazy orbit
            if (m_assignedShip == nullptr) {
                SetIdle();
                break;
            }
            if (m_pDrone->GetAuthPosition().distance(m_assignedShip->GetAuthPosition()) < m_entityOrbitRange)
                SetIdle();
        } break;
        // not sure how im gonna do these...
        case DroneAI::State::Fleeing:
        case DroneAI::State::Operating:
        case DroneAI::State::Unknown:
        case DroneAI::State::Incapacitated:
        case DroneAI::State::Guarding:
        case DroneAI::State::Assisting:
        case DroneAI::State::Combat:
        case DroneAI::State::Approaching:
        case DroneAI::State::Departing2:
        case DroneAI::State::Pursuit: {
           // do nothing here yet
        } break;

    //no default on purpose
    }
    if (sConfig.debug.UseProfiling)
        sProfiler.AddTime(Profile::drone, GetTimeUSeconds() - profileStartTime);
}

int8 DroneAIMgr::GetState() {
    switch (m_state) {
        case DroneAI::State::Invalid:
        case DroneAI::State::Unknown:
        case DroneAI::State::Incapacitated:
            return DroneAI::State::Idle;
        case DroneAI::State::Guarding:
        case DroneAI::State::Assisting:
            return DroneAI::State::Engaged;
        default:
            return m_state;
    }
}

void DroneAIMgr::Return() {
    m_assignedShip = m_pDrone->GetHomeShip();
    m_pDrone->DestinyMgr()->SetMaxVelocity(m_chaseSpeed);
    m_pDrone->DestinyMgr()->Follow(m_assignedShip, m_entityOrbitRange);
    m_state = DroneAI::State::Departing;
}

void DroneAIMgr::SetIdle() {
    m_miningRepeated = false;
    if (m_state != DroneAI::State::Idle) {
        _log(DRONE__AI_TRACE, "Drone %s(%u): SetIdle: returning to idle.",
             m_pDrone->GetName(), m_pDrone->GetID());
        m_state = DroneAI::State::Idle;

        m_webifierTimer.Disable();
        m_beginFindTarget.Disable();
        m_mainAttackTimer.Disable();
        m_warpScramblerTimer.Disable();
    }

    // Always (re)start idle orbit when entering idle — ctor state is already Idle so the
    // first LaunchDrone()->Online()->SetIdle() path must still call IdleOrbit().
    if (m_assignedShip != nullptr)
        m_pDrone->IdleOrbit(m_assignedShip);
}

bool DroneAIMgr::StartMining(SystemEntity* pTarget, bool repeatedly) {
    if ((pTarget == nullptr) or !pTarget->IsAsteroidSE())
        return false;

    if (m_assignedShip == nullptr)
        m_assignedShip = m_pDrone->GetHomeShip();
    if (m_assignedShip == nullptr)
        return false;

    bool chase = false;
    m_pDrone->TargetMgr()->ClearAllTargets();
    if (!m_pDrone->TargetMgr()->StartTargeting(
            pTarget,
            m_pDrone->GetSelf()->GetAttribute(AttrScanSpeed).get_uint32(),
            (uint8)m_pDrone->GetSelf()->GetAttribute(AttrMaxAttackTargets).get_int(),
            m_entityAttackRange,
            chase)) {
        return false;
    }

    const float cycleMs = m_pDrone->GetSelf()->GetAttribute(AttrDuration).get_float();
    m_mainAttackTimer.Start((cycleMs > 250.0f) ? cycleMs : 2500.0f);

    m_miningRepeated = repeatedly;
    m_state = DroneAI::State::Mining;
    m_pDrone->SetTarget(pTarget);
    m_pDrone->StateChange();

    m_pDrone->DestinyMgr()->SetMaxVelocity(m_chaseSpeed);
    m_pDrone->DestinyMgr()->Orbit(pTarget, PackedOrbitDistance(m_entityOrbitRange));
    return true;
}

void DroneAIMgr::SetEngaged(SystemEntity* pTarget) {
    if (m_state == DroneAI::State::Engaged && m_pDrone->DestinyMgr()->IsOrbiting())
        return;
    _log(DRONE__AI_TRACE, "Drone %s(%u): SetEngaged: %s(%u) begin engaging.",
         m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
    // actively fighting
    //   not sure of the actual orbit speed of npc's, but their 'cruise speed' seems a bit slow.
    //   this sets orbit speed between cruise speed and quarter of max speed (whether mwb or ab)
    //   this will also enable this npc to have a variable speed, instead of fixed upon creation.
    m_pDrone->DestinyMgr()->SetMaxVelocity(MakeRandomFloat(m_cruiseSpeed, (m_chaseSpeed /4)));
    m_pDrone->DestinyMgr()->Orbit(pTarget, PackedOrbitDistance(m_entityOrbitRange));  //try to get inside orbit range
    m_state = DroneAI::State::Engaged;
}

void DroneAIMgr::CheckDistance(SystemEntity* pSE)
{
    const double dist = m_pDrone->GetAuthPosition().distance(pSE->GetAuthPosition());
    const double dropDist = EvE::max(m_entityChaseRange, m_entityAttackRange * 2.5);
    DestinyManager* d = m_pDrone->DestinyMgr();

    if (dist > dropDist) {
        _log(DRONE__AI_TRACE, "Drone %s(%u): CheckDistance: %s(%u) beyond chase envelope (%.0fm > %.0fm). Clear.",
             m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID(), dist, dropDist);
        ClearTarget(pSE);
        return;
    }

    if (dist > m_entityAttackRange) {
        const bool followingSame = d->IsFollowing() && d->GetTargetID() == pSE->GetID();
        if (!followingSame) {
            d->SetMaxVelocity(m_chaseSpeed);
            d->Follow(pSE, PackedOrbitDistance(m_entityOrbitRange * 0.35));
        }
        m_state = DroneAI::State::Engaged;
        return;
    }

    if (dist < m_entityFlyRange) {
        SetEngaged(pSE);
    } else {
        const bool orbitingSame = d->IsOrbiting() && d->GetTargetID() == pSE->GetID();
        if (!orbitingSame && (d->IsFollowing() || m_state != DroneAI::State::Engaged)) {
            d->SetMaxVelocity(MakeRandomFloat(m_cruiseSpeed, (m_chaseSpeed / 4)));
            d->Orbit(pSE, PackedOrbitDistance(m_entityOrbitRange));
        }
        m_state = DroneAI::State::Engaged;
    }

    if (!m_mainAttackTimer.Enabled())
        m_mainAttackTimer.Start(m_attackSpeed);

    Attack(pSE);
}

void DroneAIMgr::ClearTargets() {
    m_pDrone->TargetMgr()->ClearTargets();
}

void DroneAIMgr::ClearAllTargets() {
    m_pDrone->TargetMgr()->ClearAllTargets();
    //m_pDrone->TargetMgr()->OnTarget(nullptr, TargMgr::Mode::Clear, TargMgr::Msg::ClientReq);
}

void DroneAIMgr::Target(SystemEntity* pTarget) {
    bool chase = false;
    if (!m_pDrone->TargetMgr()->StartTargeting(pTarget, m_pDrone->GetSelf()->GetAttribute(AttrScanSpeed).get_uint32(), (uint8)m_pDrone->GetSelf()->GetAttribute(AttrMaxAttackTargets).get_int(), m_entityAttackRange, chase)) {
        _log(DRONE__AI_TRACE, "Drone %s(%u): Targeting of %s(%u) failed.  Clear Target and Return to Idle.",
             m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
        //ClearAllTargets();
        SetIdle();
        return;
    }
    m_beginFindTarget.Disable();
    if (m_assignedShip == nullptr)
        m_assignedShip = m_pDrone->GetHomeShip();
    m_state = DroneAI::State::Engaged;
    m_pDrone->DestinyMgr()->SetMaxVelocity(m_chaseSpeed);
    CheckDistance(pTarget);

    /*
    std::map<std::string, PyRep *> arg;
    arg["target"] = new PyInt(args.arg);
    throw PyException(MakeUserError("DeniedDroneTargetForceField", arg));
    */
 //DeniedDroneTargetForceField
}

void DroneAIMgr::Targeted(SystemEntity* pAgressor) {
    _log(DRONE__AI_TRACE, "Drone %s(%u): Targeted by %s(%u) while %s.",
                m_pDrone->GetName(), m_pDrone->GetID(), pAgressor->GetName(), pAgressor->GetID(), GetStateName(m_state).c_str());
    switch(m_state) {
        case DroneAI::State::Idle: {
        } break;
        case DroneAI::State::Operating: {
        } break;
        case DroneAI::State::Unknown: {
        } break;
        case DroneAI::State::Engaged: {
        } break;
        case DroneAI::State::Fleeing: {
        } break;
        case DroneAI::State::Incapacitated: {
        } break;
        case DroneAI::State::Guarding: {
        } break;
        case DroneAI::State::Assisting: {
        } break;
        case DroneAI::State::Combat: {
        } break;
        case DroneAI::State::Mining: {
        } break;
        case DroneAI::State::Approaching: {
        } break;
        case DroneAI::State::Departing: {
        } break;
        case DroneAI::State::Departing2: {
        } break;
        case DroneAI::State::Pursuit: {
        } break;
    }
}

void DroneAIMgr::TargetLost(SystemEntity* pTarget) {
    switch(m_state) {
        case DroneAI::State::Engaged: {
            if (m_pDrone->TargetMgr()->HasNoTargets()) {
                _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) lost. No targets remain.  Return to Idle.",
                     m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
                SetIdle();
            } else {
                _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) lost, but more targets remain.",
                     m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
            }

        } break;
        case DroneAI::State::Mining: {
            if (m_pDrone->TargetMgr()->HasNoTargets())
                SetIdle();
        } break;

        default:
            break;
    }
}

void DroneAIMgr::Attack(SystemEntity* pSE)
{
    if (m_mainAttackTimer.Check()) {
        if (pSE == nullptr)
            return;
        // Check to see if the target still in the bubble (Client warped out)
        // fighters/bombers are able to follow.
        if (!m_pDrone->SysBubble()->InBubble(pSE->GetAuthPosition())) {
            _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) no longer in bubble.  Clear target and move on",
                 m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID());
            ClearTarget(pSE);
            return;
        }
        DestinyManager* pDestiny = pSE->DestinyMgr();
        if (pDestiny == nullptr) {
            _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) has no destiny manager.  Clear target and move on",
                 m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID());
            ClearTarget(pSE);
            return;
        }
        // Check to see if the target is not cloaked:
        if (pDestiny->IsCloaked()) {
            _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) is cloaked.  Clear target and move on",
                 m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID());
            ClearTarget(pSE);
            return;
        }

        if (m_pDrone->TargetMgr()->CanAttack())
            AttackTarget(pSE);
    }
}

void DroneAIMgr::Mine(SystemEntity* pSE)
{
    if ((pSE == nullptr) or !pSE->IsAsteroidSE()) {
        SetIdle();
        return;
    }
    if (m_assignedShip == nullptr)
        m_assignedShip = m_pDrone->GetHomeShip();
    if (m_assignedShip == nullptr) {
        SetIdle();
        return;
    }

    const float range = EvE::max(500.0f, m_pDrone->GetSelf()->GetAttribute(AttrMaxRange).get_float());
    const double distance = m_pDrone->GetAuthPosition().distance(pSE->GetAuthPosition());
    if (distance > range) {
        m_pDrone->DestinyMgr()->Orbit(pSE, PackedOrbitDistance(static_cast<double>(range) * 0.8));
        return;
    }

    if (!m_mainAttackTimer.Check())
        return;

    ShipItemRef shipRef = m_assignedShip->GetShipItemRef();
    if (shipRef.get() == nullptr) {
        SetIdle();
        return;
    }

    InventoryItemRef roidRef(pSE->GetSelf());
    const float cycleVol = m_pDrone->GetSelf()->GetAttribute(AttrMiningAmount).get_float();
    const float oreVolume = roidRef->GetAttribute(AttrVolume).get_float();
    if ((cycleVol <= 0.0f) or (oreVolume <= 0.0f)) {
        SetIdle();
        return;
    }

    float oreAmount = (cycleVol / oreVolume);
    float roidQuantity = roidRef->GetAttribute(AttrQuantity).get_float();
    if (roidQuantity <= 0.0f) {
        SetIdle();
        return;
    }
    if (oreAmount > roidQuantity)
        oreAmount = roidQuantity;

    float remainingCargoVolume = shipRef->GetRemainingVolumeByFlag(flagCargoHold);
    if (remainingCargoVolume < cycleVol) {
        if (remainingCargoVolume > oreVolume) {
            oreAmount = remainingCargoVolume / oreVolume;
        } else {
            if (m_pDrone->GetOwner() != nullptr)
                m_pDrone->GetOwner()->SendNotifyMsg("Your mining drone deactivates because your cargohold is full.");
            SetIdle();
            return;
        }
    }
    if (oreAmount <= 0.0f) {
        SetIdle();
        return;
    }

    roidQuantity -= oreAmount;
    if (roidQuantity > 0.0f) {
        roidRef->SetAttribute(AttrQuantity, roidQuantity);
        // reverse radius->quantity formula used for asteroids
        roidRef->SetAttribute(AttrRadius, exp((roidQuantity + 112404.8) / 25000));
    }

    ItemData idata(roidRef->typeID(), shipRef->ownerID(), locTemp, flagNone, oreAmount);
    InventoryItemRef oreRef(sItemFactory.SpawnItem(idata));
    if (oreRef.get() == nullptr) {
        SetIdle();
        return;
    }
    if (!shipRef->AddItemByFlag(flagCargoHold, oreRef)) {
        SetIdle();
        return;
    }
    if (roidQuantity <= 0.0f) {
        m_pDrone->TargetMgr()->ClearTarget(pSE);
        pSE->Delete();
        SafeDelete(pSE);
        SetIdle();
        return;
    }

    if (!m_miningRepeated)
        SetIdle();
}

void DroneAIMgr::ClearTarget(SystemEntity* pSE) {
    m_pDrone->TargetMgr()->ClearTarget(pSE);
    //m_pDrone->TargetMgr()->OnTarget(pSE, TargMgr::Mode::Lost);

    if (m_pDrone->TargetMgr()->HasNoTargets())
        SetIdle();
}

//also check for special effects and write code to implement them
//modifyTargetSpeedRange, modifyTargetSpeedChance
//entityWarpScrambleChance

void DroneAIMgr::AttackTarget(SystemEntity* pTarget) {
    /** @todo  not all drones use lazors...fix this */
    //  woot!! --> group:1010        cat:8       Compact Citadel Torpedo         Citadel torpedoes for fighter-bombers

    // effects are listed in EVE_Effects.h
    //  NOTE: drones are called 'entities' in client; EVE_Effects has 'entityxxx' for gfx
    std::string guid = "effects.Laser"; // client looks for 'turret' in ship.ball.modules for 'effects.laser'
    //effects.ProjectileFiredForEntities
    uint32 gfxID = 0;
    if (m_pDrone->GetSelf()->HasAttribute(AttrGfxTurretID))// graphicID for turret for drone type ships
        gfxID = m_pDrone->GetSelf()->GetAttribute(AttrGfxTurretID).get_uint32();
    m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->typeID(), //m_pDrone->GetSelf()->GetAttribute(AttrGfxTurretID).get_int(),
                                             pTarget->GetID(),
                                             0,guid,1,1,1,m_attackSpeed,0,gfxID);

    Damage d(m_pDrone,
             m_pDrone->GetSelf(),
             m_pDrone->GetKinetic(),
             m_pDrone->GetThermal(),
             m_pDrone->GetEM(),
             m_pDrone->GetExplosive(),
             m_formula.GetDroneToHit(m_pDrone, pTarget),
             EVEEffectID::targetAttack
            );

    d *= m_pDrone->GetSelf()->GetAttribute(AttrDamageMultiplier).get_float();
    d *= sConfig.rates.damageRate;      /** @todo this should be a separate config value */
    pTarget->ApplyDamage(d);
}


std::string DroneAIMgr::GetStateName(int8 stateID)
{
    switch (stateID) {
        case DroneAI::State::Idle:            return "Idle";
        case DroneAI::State::Combat:          return "Combat";
        case DroneAI::State::Mining:          return "Mining";
        case DroneAI::State::Approaching:     return "Approaching";
        case DroneAI::State::Departing:       return "Returning to ship";
        case DroneAI::State::Departing2:      return "Departing2";
        case DroneAI::State::Pursuit:         return "Pursuit";
        case DroneAI::State::Engaged:         return "Engaged";
        case DroneAI::State::Fleeing:         return "Fleeing";
        case DroneAI::State::Unknown:         return "Unknown";
        case DroneAI::State::Operating:       return "Operating";
        case DroneAI::State::Assisting:       return "Assisting";
        case DroneAI::State::Guarding:        return "Guarding";
        case DroneAI::State::Incapacitated:   return "Incapacitated";
        default:                              return "Invalid";
    }
}
