 /**
  * @name CynoModule.cpp
  *   Cynosural field generator module class
  * @Author: James
  * @date:   8 October 2021
  */

#include "ship/modules/CynoModule.h"
#include "inventory/AttributeEnum.h"
#include "inventory/Inventory.h"
#include "map/MapDB.h"
#include "system/SystemManager.h"
#include "fleet/FleetService.h"
#include "pos/Tower.h"
#include "system/sov/SovereigntyDataMgr.h"

namespace {

/** Used when static data omits AttrConsumptionType (Liquid Ozone per cycle). */
constexpr uint32 LIQUID_OZONE_PER_CYCLE_FALLBACK = 400;

uint32 LiquidOzoneQtyPerCycle(const ModuleItemRef& mod)
{
    if (mod->HasAttribute(AttrConsumptionType) && mod->HasAttribute(AttrConsumptionQuantity)) {
        const uint32 ctype = mod->GetAttribute(AttrConsumptionType).get_uint32();
        if (ctype == EVEDB::invTypes::LiquidOzone) {
            const uint32 q = mod->GetAttribute(AttrConsumptionQuantity).get_uint32();
            return q < 1u ? 1u : q;
        }
        return 0;
    }
    return LIQUID_OZONE_PER_CYCLE_FALLBACK;
}

/** Remove qty of type from cargo hold (may span multiple stacks). */
bool ConsumeFromCargoHold(ShipItemRef ship, uint16 typeID, uint32 qtyNeed)
{
    if (!ship->GetMyInventory()->ContainsTypeQtyByFlag(typeID, flagCargoHold, qtyNeed))
        return false;

    uint32 quantityLeft = qtyNeed;
    std::vector<InventoryItemRef> stacks;
    ship->GetMyInventory()->GetItemsByFlag(flagCargoHold, stacks);
    for (auto cur : stacks) {
        if (cur->typeID() != typeID)
            continue;
        if (cur->quantity() >= quantityLeft) {
            cur->AlterQuantity(-(int32)quantityLeft, true);
            break;
        }
        quantityLeft -= cur->quantity();
        cur->SetQuantity(0, true, true);
        if (quantityLeft < 1)
            break;
    }
    return quantityLeft < 1;
}

} // namespace

CynoModule::CynoModule(ModuleItemRef mRef, ShipItemRef sRef,
                       uint32 fieldTypeID,
                       bool requiresFleet,
                       bool blockedBySovJammer)
: ActiveModule(mRef, sRef),
m_fieldTypeID(fieldTypeID),
m_requiresFleet(requiresFleet),
m_blockedBySovJammer(blockedBySovJammer),
pClient(nullptr),
pShipSE(nullptr),
cSE(nullptr),
m_firstRun(true),
m_shipVelocity(0.0f)
{
    if (!m_shipRef->HasPilot())
        return;

    pClient = m_shipRef->GetPilot();

    // increase scan speed by level of survey skill
    float cycleTime = GetAttribute(AttrDuration).get_float();
    cycleTime *= (1 + (0.03f * (pClient->GetChar()->GetSkillLevel(EvESkill::Survey, true))));
    SetAttribute(AttrDuration, cycleTime);
}

void CynoModule::Activate(uint16 effectID, uint32 targetID, int16 repeat)
{
    pShipSE = pClient->GetShipSE();

    m_firstRun = true;
    ActiveModule::Activate(effectID, targetID, repeat);

    // check to see if Activate() was denied.
    if (m_Stop)
        return;

    _log(MODULE__DEBUG, "Cynosural field generator activated by %s in %s", pClient->GetName(), m_sysMgr->GetName());

    // hack to disable ship movement here
    m_shipVelocity = pShipSE->DestinyMgr()->GetMaxVelocity();
    pShipSE->DestinyMgr()->SetFrozen(true);
}

void CynoModule::DeactivateCycle(bool abort)
{
    SendOnJumpBeaconChange(false);

    const uint32 beaconID = (cSE != nullptr ? cSE->GetID() : 0);
    if (beaconID != 0)
        sFltSvc.InvalidateBridgesToBeacon(beaconID);

    if (pClient != nullptr && cSE != nullptr) {
        sFltSvc.UnregisterActiveBeacon(pClient->GetCharacterID());
        MapDB::AdjustCynoModuleCount(pClient->GetLocationID(), -1);
    }

    if (cSE != nullptr) {
        cSE->Delete();
        SafeDelete(cSE);
    }
    ActiveModule::DeactivateCycle(abort);

    // hack to reinstate ship movement here
    // may have to reset/reapply all fx for ship movement
    if (pShipSE != nullptr) {
        pShipSE->GetSelf()->SetAttribute(AttrMaxVelocity, m_shipVelocity);
        pShipSE->DestinyMgr()->SetFrozen(false);
    }
}

bool CynoModule::CanActivate()
{
    if (m_requiresFleet && !pClient->InFleet())
        throw UserError("CynoMustBeInFleet");

    if (pShipSE->SysBubble()->HasTower()) {
        TowerSE* ptSE = pShipSE->SysBubble()->GetTowerSE();
        if (ptSE->HasForceField())
            if (pShipSE->GetAuthPosition().distance(ptSE->GetAuthPosition()) < ptSE->GetSOI())
                throw UserError("NoCynoInPOSShields");
    }

    SovereigntyData sovData = svDataMgr.GetSovereigntyData(pClient->GetLocationID());
    if (sConfig.world.enforceSovJammer && m_blockedBySovJammer && sovData.jammerID != 0)
        throw UserError("CynosuralGenerationJammed");

    // Make sure player is not in high-sec (configurable). Compare map security, not internal GetSecValue().
    if (!sConfig.world.highSecCyno) {
        if (pClient->SystemMgr()->GetMapSecurityStatus() >= 0.5f) {
            pClient->SendNotifyMsg("This module may not be used in high security space.");
            return false;
        }
    }

    /* Liquid ozone: ActiveModule::CanActivate only checks AttrConsumption* when present in dogma.
     * If missing, enforce a per-cycle LOZ requirement (matches client / live behavior). */
    const uint32 lozQty = LiquidOzoneQtyPerCycle(m_modRef);
    if (!m_modRef->HasAttribute(AttrConsumptionType)) {
        if (!m_shipRef->GetMyInventory()->ContainsTypeQtyByFlag(
                EVEDB::invTypes::LiquidOzone, flagCargoHold, lozQty)) {
            pClient->SendNotifyMsg(
                "This module requires you to have %u units of %s in your cargo hold.",
                lozQty,
                sItemFactory.GetType(EVEDB::invTypes::LiquidOzone)->name().c_str());
            return false;
        }
    }

    // all specific checks pass.  run generic checks in base class
    return ActiveModule::CanActivate();
}

uint32 CynoModule::DoCycle()
{
    // i really dont like this, but cant get it to work anywhere else...
    uint32 retVal(0);
    if (retVal = ActiveModule::DoCycle())
        if (m_firstRun) {
            m_firstRun = false;
            CreateCyno();
        }

    return retVal;
}

void CynoModule::CreateCyno()
{
    // do we already have a cyno field created?
    if (cSE != nullptr)
        return;

    /* When dogma has no consumption attributes, deduct LOZ here (DoCycle skips in that case). */
    if (!m_modRef->HasAttribute(AttrConsumptionType)) {
        const uint32 lozQty = LiquidOzoneQtyPerCycle(m_modRef);
        if (!ConsumeFromCargoHold(m_shipRef, EVEDB::invTypes::LiquidOzone, lozQty)) {
            m_shipRef->GetPilot()->SendNotifyMsg(
                "This module requires you to have %u units of %s in your cargo hold.",
                lozQty,
                sItemFactory.GetType(EVEDB::invTypes::LiquidOzone)->name().c_str());
            AbortCycle();
            return;
        }
    }

    ItemData cData(m_fieldTypeID, pClient->GetCharacterID(), m_sysMgr->GetID(), flagNone);
    InventoryItemRef cRef = sItemFactory.SpawnItem(cData);

    _log(MODULE__DEBUG, "Creating Cynosural field (type %u)", m_fieldTypeID);

    cSE = new ItemSystemEntity(cRef, pClient->services(), m_sysMgr);
    GPoint location(pShipSE->GetPosition());
    location.MakeRandomPointOnSphere(1500.0f + cRef->type().radius());
    cSE->SetPosition(location);
    cRef->SaveItem();
    m_sysMgr->AddEntity(cSE);

    const bool covert = (m_fieldTypeID == EVEDB::invTypes::CovertCynosuralFieldI);
    sFltSvc.RegisterActiveBeacon(pClient->GetCharacterID(), cRef->itemID(), m_sysMgr->GetID(), covert);
    MapDB::AdjustCynoModuleCount(m_sysMgr->GetID(), 1);

    SendOnJumpBeaconChange(true);
}

void CynoModule::SendOnJumpBeaconChange(bool active/*false*/) {
        //Send ProcessSovStatusChanged Notification
        _log(MODULE__DEBUG, "Sending OnJumpBeaconChange (active = %s)", active ? "true" : "false");

        uint32 fieldID(0);
        if (cSE != nullptr)
            fieldID = cSE->GetID();

        PyTuple* data = new PyTuple(4);
            data->SetItem(0, new PyInt(pClient->GetCharacterID()));
            data->SetItem(1, new PyInt(m_sysMgr->GetID()));
            data->SetItem(2, new PyInt(fieldID));
            data->SetItem(3, new PyBool(active));

        if (!pClient->InFleet())
            return;

        std::vector<Client *> fleetClients;
        fleetClients = sFltSvc.GetFleetClients(pClient->GetFleetID());
        for (auto cur : fleetClients)
            if (cur != nullptr) {
                cur->SendNotification("OnJumpBeaconChange", "clientID", &data);
                _log(MODULE__DEBUG, "OnJumpBeaconChange sent to %s (%u)", cur->GetName(), cur->GetCharID());
            }
}

/*
 * {'messageKey': 'CynoMustBeInFleet', 'dataID': 17879144, 'suppressable': False, 'bodyID': 257897, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 257896, 'messageID': 2253}
 * {'messageKey': 'CynosuralGenerationJammed', 'dataID': 17880043, 'suppressable': False, 'bodyID': 258240, 'messageType': 'notify', 'urlAudio': '', 'urlIcon': '', 'titleID': None, 'messageID': 2249}
 * {'messageKey': 'NoActiveBeacon', 'dataID': 17879973, 'suppressable': False, 'bodyID': 258214, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 258213, 'messageID': 2211}
 * {'messageKey': 'NoCynoInPOSShields', 'dataID': 17878872, 'suppressable': False, 'bodyID': 257791, 'messageType': 'notify', 'urlAudio': '', 'urlIcon': '', 'titleID': None, 'messageID': 2339}
 *
 */

/*
 * {'FullPath': u'UI/Messages', 'messageID': 257896, 'label': u'CynoMustBeInFleetTitle'}(u'Cannot start Cynosural Field', None, None)
 * {'FullPath': u'UI/Messages', 'messageID': 257897, 'label': u'CynoMustBeInFleetBody'}(u'You must be in a fleet to activate your Cynosural Field.', None, None)
 *
 * {'FullPath': u'UI/Messages', 'messageID': 258213, 'label': u'NoActiveBeaconTitle'}(u'No Active Beacon', None, None)
 * {'FullPath': u'UI/Messages', 'messageID': 258214, 'label': u'NoActiveBeaconBody'}(u'You do not have an active Cynosural Field Beacon.', None, None)
 *
 * {'FullPath': u'UI/Messages', 'messageID': 258240, 'label': u'CynosuralGenerationJammedBody'}(u'You are unable to generate a cynosural field while there is a jammer present and active somewhere in this solar system.', None, None)
 *
 * {'FullPath': u'UI/Messages', 'messageID': 257791, 'label': u'NoCynoInPOSShieldsBody'}(u'You cannot set up a cynosural field within the force field of a Control Tower', None, None)
 */
