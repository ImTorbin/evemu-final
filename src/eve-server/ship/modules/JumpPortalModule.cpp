
#include "eve-server.h"

#include "utils/utils_time.h"
#include "fleet/FleetService.h"
#include "inventory/AttributeEnum.h"
#include "pos/Tower.h"
#include "ship/modules/JumpPortalModule.h"
#include "tables/invGroups.h"

JumpPortalModule::JumpPortalModule(ModuleItemRef mRef, ShipItemRef sRef)
: ActiveModule(mRef, sRef),
  m_pClient(nullptr),
  m_requireCovert(false)
{
    if (m_modRef->HasAttribute(AttrJumpPortalDuration))
        SetAttribute(AttrDuration, m_modRef->GetAttribute(AttrJumpPortalDuration));
}

bool JumpPortalModule::CanActivate()
{
    m_pClient = m_shipRef->GetPilot();
    if (m_pClient == nullptr)
        return false;

    ShipSE* pShipSE = m_pClient->GetShipSE();
    if (pShipSE == nullptr)
        return false;

    const uint32 sg = m_shipRef->groupID();
    if (sg != EVEDB::invGroups::Titan && sg != EVEDB::invGroups::BlackOps) {
        throw CustomError("This jump portal can only be operated from a Titan or Black Ops hull.");
    }

    m_requireCovert = (sg == EVEDB::invGroups::BlackOps);

    if (m_pClient->GetChar()->GetSkillLevel(EvESkill::JumpPortalGeneration, true) < 1)
        throw CustomError("You need the Jump Portal Generation skill to use this module.");

    if (!m_pClient->InFleet())
        throw UserError("CynoMustBeInFleet");

    if (!IsValidTarget(m_targetID) || m_targetSE == nullptr || !m_targetSE->IsShipSE())
        throw UserError("DeniedActivateTargetNotPresent");

    ShipSE* tgtShip = m_targetSE->GetShipSE();
    if (tgtShip == nullptr)
        throw CustomError("Invalid jump portal target.");

    Client* tgtPilot = tgtShip->GetPilot();
    if (tgtPilot == nullptr)
        throw CustomError("Jump portal requires a piloted fleet ship as target.");

    if (tgtPilot->GetFleetID() != m_pClient->GetFleetID())
        throw CustomError("Jump portal target must be in your fleet.");

    if (pShipSE->SysBubble()->HasTower()) {
        TowerSE* ptSE = pShipSE->SysBubble()->GetTowerSE();
        if (ptSE->HasForceField())
            if (pShipSE->GetAuthPosition().distance(ptSE->GetAuthPosition()) < ptSE->GetSOI())
                throw UserError("NoCynoInPOSShields");
    }

    if (!sConfig.world.highSecCyno) {
        if (m_pClient->SystemMgr()->GetMapSecurityStatus() >= 0.5f) {
            m_pClient->SendNotifyMsg("This module may not be used in high security space.");
            return false;
        }
    }

    FleetBeaconEntry be;
    if (!sFltSvc.FindCompatibleFleetBeacon(m_pClient->GetFleetID(), m_requireCovert, be)) {
        m_pClient->SendNotifyMsg("No compatible active cynosural beacon is available in your fleet.");
        return false;
    }

    return ActiveModule::CanActivate();
}

void JumpPortalModule::Activate(uint16 effectID, uint32 targetID, int16 repeat)
{
    m_pClient = m_shipRef->GetPilot();

    ActiveModule::Activate(effectID, targetID, repeat);

    if (m_Stop || m_pClient == nullptr)
        return;

    FleetBeaconEntry be;
    if (!sFltSvc.FindCompatibleFleetBeacon(m_pClient->GetFleetID(), m_requireCovert, be)) {
        m_pClient->SendNotifyMsg("Fleet beacon no longer available; jump portal aborted.");
        AbortCycle();
        return;
    }

    double totalMass = 5000000000.0;
    double factor = 1.0;
    if (m_modRef->HasAttribute(AttrJumpPortalConsumptionMassFactor)) {
        factor = m_modRef->GetAttribute(AttrJumpPortalConsumptionMassFactor).get_double();
        if (factor <= 0.0)
            factor = 1.0;
    }

    int64 durMs = 600000;
    if (m_modRef->HasAttribute(AttrJumpPortalDuration))
        durMs = (int64)m_modRef->GetAttribute(AttrJumpPortalDuration).get_float();
    else if (m_modRef->HasAttribute(AttrDuration))
        durMs = (int64)m_modRef->GetAttribute(AttrDuration).get_float();
    if (durMs < 1000)
        durMs = 600000;

    const int64 expiry = GetSteadyTime() + durMs;

    sFltSvc.RegisterJumpPortal(m_shipRef->itemID(), be.beaconItemID, be.solarSystemID,
                                 m_requireCovert, totalMass, factor, expiry);

    _log(MODULE__DEBUG, "Jump portal registered: bridgeShip %u beacon %u system %u covert=%s",
         m_shipRef->itemID(), be.beaconItemID, be.solarSystemID, m_requireCovert ? "yes" : "no");
}

void JumpPortalModule::DeactivateCycle(bool abort)
{
    sFltSvc.UnregisterJumpPortal(m_shipRef->itemID());
    ActiveModule::DeactivateCycle(abort);
}
