/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:        Bloody.Rabbit
    Update/Rewrite: Allan
*/

#include "eve-server.h"

#include "StaticDataMgr.h"
#include "system/Celestial.h"
#include "system/SystemManager.h"

/*
 * CelestialObject
 */
CelestialObject::CelestialObject(uint32 _celestialID, const ItemType &_type, const ItemData &_data)
: InventoryItem(_celestialID, _type, _data),
  m_radius( 0.0 ),
  m_security( 0.0 ),
  m_celestialIndex( 0 ),
  m_orbitIndex( 0 )
  {
      _log(ITEM__TRACE, "Created Default CelestialObject %p for item %s (%u).", this, name(), itemID());
}

CelestialObject::CelestialObject(uint32 _celestialID, const ItemType &_type, const ItemData &_data, const CelestialObjectData &_cData)
: InventoryItem(_celestialID, _type, _data),
  m_radius(_cData.radius),  // no longer needed here
  m_security(_cData.security),
  m_celestialIndex(_cData.celestialIndex),
  m_orbitIndex(_cData.orbitIndex)
{
      _log(ITEM__TRACE, "Created CelestialObject %p for %s (%u) with radius of %.1f.", this, name(), itemID(), m_radius);
}

CelestialObjectRef CelestialObject::Load( uint32 celestialID)
{
    return InventoryItem::Load<CelestialObject>(celestialID );
}

CelestialObjectRef CelestialObject::Spawn( ItemData &data) {
    uint32 celestialID = CelestialObject::CreateItemID(data);
    if (celestialID == 0 )
        return CelestialObjectRef(nullptr);

    CelestialObjectRef celestialRef = CelestialObject::Load(celestialID);

    if ((celestialRef->type().groupID() == EVEDB::invGroups::Beacon)
    or  (celestialRef->type().groupID() == EVEDB::invGroups::Effect_Beacon))
        celestialRef->SetAttribute(AttrIsGlobal, EvilOne);

    return celestialRef;
}

uint32 CelestialObject::CreateItemID( ItemData &data) {
    return InventoryItem::CreateItemID(data);
}

void CelestialObject::Delete() {
    InventoryItem::Delete();
}


CelestialSE::CelestialSE(InventoryItemRef self, EVEServiceManager &services, SystemManager* system)
: ItemSystemEntity(self, services, system)
{
    _log(SE__DEBUG, "Created CSE for item %s (%u) with radius of %.1f.", self->name(), self->itemID(), m_radius);
}

void CelestialSE::EncodeDestiny(Buffer& into)
{
    using namespace Destiny;
    BallHeader head = BallHeader();
        head.entityID = m_self->itemID();
        head.mode = Ball::Mode::RIGID;
        head.radius = m_radius;
        head.posX = x();
        head.posY = y();
        head.posZ = z();
        // Like StaticSystemEntity: system-wide rigid balls. ItemSystemEntity used flags=0 so planets/moons
        // were bubble-local; client warp FX (effects.warp GetWarpCollisions) then had planetBall=None and
        // crashed on planetBall.radius when ray-testing the warp line.
        head.flags = Ball::Flag::IsGlobal;
    into.Append(head);
    RIGID_Struct main;
        main.formationID = 0xFF;
    into.Append(main);

    _log(SE__DESTINY, "CSE::EncodeDestiny(): %s - id:%lli, mode:%u, flags:0x%X, radius:%.1f",
        GetName(), head.entityID, head.mode, head.flags, head.radius);
}

void CelestialSE::MakeDamageState(DoDestinyDamageState &into)
{
    double shield = 1.0, armor = 1.0, structure = 1.0, recharge = 1000000;
    if (m_self->HasAttribute(AttrShieldRechargeRate))
        recharge = m_self->GetAttribute(AttrShieldRechargeRate).get_float();
    if (m_self->HasAttribute(AttrShieldCharge) and m_self->HasAttribute(AttrShieldCapacity))
        shield = (m_self->GetAttribute(AttrShieldCharge).get_float() / m_self->GetAttribute(AttrShieldCapacity).get_float());
    if (m_self->HasAttribute(AttrArmorDamage) and m_self->HasAttribute(AttrArmorHP))
        armor = 1.0 - (m_self->GetAttribute(AttrArmorDamage).get_float() / m_self->GetAttribute(AttrArmorHP).get_float());
    if (m_self->HasAttribute(AttrDamage) and m_self->HasAttribute(AttrHP))
        structure = 1.0 - (m_self->GetAttribute(AttrDamage).get_float() / m_self->GetAttribute(AttrHP).get_float());

    into.armor = armor;
    into.shield = shield;
    into.recharge = recharge;
    into.structure = structure;
    into.timestamp = GetFileTimeNow();
}


AnomalySE::AnomalySE(CelestialObjectRef self, EVEServiceManager& services, SystemManager* system)
: CelestialSE(self, services, system)
{
}

void AnomalySE::EncodeDestiny(Buffer& into)
{
    using namespace Destiny;
    BallHeader head = BallHeader();
        head.entityID = m_self->itemID();
        head.mode = Ball::Mode::STOP;
        head.radius = m_radius;
        head.posX = x();
        head.posY = y();
        head.posZ = z();
        /* System-wide ball: flags=0 kept the anomaly out of the global ballpark; client warp FX
         * (effects.warp GetWarpCollisions) then failed ball lookup (planetBall=None) and crashed
         * on .radius when ray-testing the warp line. Match CelestialSE / planets (IsGlobal). */
        head.flags = Ball::Flag::IsGlobal;
    into.Append( head );
    MassSector mass = MassSector();
        mass.mass = 10000000000;    // as seen in packets
        mass.cloak = 0;
        mass.harmonic = m_harmonic;
        mass.corporationID = -1;
        mass.allianceID = -1;
    into.Append( mass );
    STOP_Struct main;
        main.formationID = 0xFF;
    into.Append( main );

    _log(SE__DESTINY, "AnomalySE::EncodeDestiny(): %s - id:%lli, mode:%u, flags:0x%X", GetName(), head.entityID, head.mode, head.flags);
}

PyDict* AnomalySE::MakeSlimItem()
{
    _log(SE__SLIMITEM, "MakeSlimItem for AnomalySE %s(%u)", GetName(), m_self->itemID());
    PyDict *slim = new PyDict();
        slim->SetItemString("ballID",           new PyLong(m_self->itemID()));
        slim->SetItemString("itemID",           new PyLong(m_self->itemID()));
        slim->SetItemString("typeID",           new PyInt(m_self->typeID()));
        slim->SetItemString("dungeonDataID",    new PyInt(0)); //?  seen 2990651
        slim->SetItemString("ownerID",          new PyInt(m_ownerID));
        if (m_radius > 0.0)
            slim->SetItemString("radius",       new PyFloat(m_radius));
    return slim;
}

WormholeSE::WormholeSE(CelestialObjectRef self, EVEServiceManager& services, SystemManager* system)
: CelestialSE(self, services, system),
m_count(0),
m_wormholeAge(WormHole::Age::Adolescent),
// no clue what this is...may not be used.  seen 33, 263, 27 in logs
m_dunSpawnID(0)
{
    m_wormholeSize = (WormHole::Size::Full /10);
    // just guessing here....
    m_expiryDate = Win32TimeNow() + EvE::Time::Day;
    // data found in eveGraphics table.  yes.  11781 - 11786 (class 1-6)  -3715 doesnt work
    m_nebulaType = 11785;
}

void WormholeSE::EncodeDestiny(Buffer& into)
{
    using namespace Destiny;
    BallHeader head = BallHeader();
        head.entityID = m_self->itemID();
        head.mode = Ball::Mode::STOP;
        head.radius = m_radius;
        head.posX = x();
        head.posY = y();
        head.posZ = z();
        head.flags = Ball::Flag::IsGlobal;
    into.Append( head );
    MassSector mass = MassSector();
        mass.mass = 10000000000;    // as seen in packets
        mass.cloak = 0;
        mass.harmonic = m_harmonic;
        mass.corporationID = -1;
        mass.allianceID = -1;
    into.Append( mass );
    STOP_Struct main;
        main.formationID = 0xFF;
    into.Append( main );
    _log(SE__DESTINY, "WormholeSE::EncodeDestiny(): %s - id:%lli, mode:%u, flags:0x%X", GetName(), head.entityID, head.mode, head.flags);
}

PyDict* WormholeSE::MakeSlimItem()
{
    _log(SE__SLIMITEM, "MakeSlimItem for WormholeSE %s(%u)", GetName(), m_self->itemID());
    PyDict *slim = new PyDict();
        slim->SetItemString("ballID",                   new PyLong(m_self->itemID()));
        slim->SetItemString("itemID",                   new PyLong(m_self->itemID()));
        slim->SetItemString("typeID",                   new PyInt(m_self->typeID()));
        slim->SetItemString("ownerID",                  new PyInt(m_ownerID));
        if (m_radius > 0.0)
            slim->SetItemString("radius",               new PyFloat(m_radius));
        slim->SetItemString("otherSolarSystemClass",    new PyInt(sDataMgr.GetWHSystemClass(m_system->GetID())));
        slim->SetItemString("wormholeSize",             new PyFloat(m_wormholeSize));
        slim->SetItemString("wormholeAge",              new PyInt(m_wormholeAge));
        slim->SetItemString("count",                    new PyInt(m_count));   //ships jumped thru?
        slim->SetItemString("dunSpawnID",               new PyInt(m_dunSpawnID));
        slim->SetItemString("nebulaType",               new PyInt(m_nebulaType));
        slim->SetItemString("expiryDate",               new PyLong(m_expiryDate));
    return slim;
}
