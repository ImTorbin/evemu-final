#include "eve-server.h"

#include "Client.h"
#include "ServiceDB.h"
#include "system/CQManager.h"

uint32 CQManager::EnsureWorldSpaceForStation(uint32 stationID) {
    uint32 worldSpaceID = ServiceDB::GetOrCreateCQWorldSpace(stationID);
    if (worldSpaceID == 0) {
        sLog.Error("CQ", "[CQ] EnsureWorldSpaceForStation: DB returned 0 for station %u", stationID);
        return 0;
    }

    auto it = m_worldspaces.find(worldSpaceID);
    if (it == m_worldspaces.end()) {
        CQWorldSpace ws;
        ws.worldSpaceID = worldSpaceID;
        ws.stationID = stationID;
        m_worldspaces.emplace(worldSpaceID, ws);
        sLog.Green("CQ", "[CQ] EnsureWorldSpaceForStation: created in-memory worldSpace %u for station %u", worldSpaceID, stationID);
    } else {
        sLog.Green("CQ", "[CQ] EnsureWorldSpaceForStation: reuse worldSpace %u for station %u (occupants=%zu)",
            worldSpaceID, stationID, it->second.occupants.size());
    }
    return worldSpaceID;
}

uint32 CQManager::Join(Client* client, uint32 stationID) {
    if (client == nullptr) {
        return 0;
    }

    const uint32 worldSpaceID = EnsureWorldSpaceForStation(stationID);
    if (worldSpaceID == 0) {
        sLog.Error("CQ", "[CQ] Join: failed ensure worldspace (char=%u station=%u)", client->GetCharacterID(), stationID);
        return 0;
    }
    auto& ws = m_worldspaces[worldSpaceID];

    CQOccupantState state;
    state.characterID = client->GetCharacterID();
    state.stationID = stationID;
    state.worldSpaceID = worldSpaceID;
    state.position = GPoint();
    state.corporationID = client->GetCorporationID();
    CharacterRef ch = client->GetChar();
    if (ch.get() != nullptr) {
        state.bloodlineID = ch->bloodlineID();
        state.raceID = static_cast<uint32>(ch->race());
        state.genderID = ch->gender() ? 1u : 0u;
    }
    ws.occupants[state.characterID] = state;
    m_playerToWorldspace[state.characterID] = worldSpaceID;

    ServiceDB::SetCQOccupancy(worldSpaceID, state.characterID, true);
    sLog.Green("CQ", "[CQ] Join: char=%u (%s) station=%u worldSpace=%u occupantsNow=%zu (broadcast spawn to others)",
        state.characterID, client->GetName(), stationID, worldSpaceID, ws.occupants.size());

    // Compatibility fallback: some clients ignore CQ-specific movement notifies.
    // Re-emit standard station presence notify so occupant lists/avatars refresh.
    OnCharNowInStation ocnis;
    ocnis.charID = state.characterID;
    ocnis.corpID = client->GetCorporationID();
    ocnis.allianceID = client->GetAllianceID();
    ocnis.warFactionID = client->GetWarFactionID();
    PyTuple* stationPresence = ocnis.Encode();
    for (const auto& entry : ws.occupants) {
        if (entry.first == state.characterID) {
            continue;
        }
        Client* target = sEntityList.FindClientByCharID(entry.first);
        if (target == nullptr) {
            continue;
        }
        PySafeIncRef(stationPresence);
        target->SendNotification("OnCharNowInStation", "stationid", &stationPresence);
        sLog.Green("CQ", "[CQ] Join: sent OnCharNowInStation fallback to char=%u for joinedChar=%u",
            entry.first, state.characterID);
    }
    PySafeDecRef(stationPresence);

    // Reciprocal fallback: inform the newly joined client about occupants that
    // were already in this shared CQ. Without bound CQ snapshot calls, this helps
    // the client refresh presence for incumbents.
    for (const auto& entry : ws.occupants) {
        if (entry.first == state.characterID) {
            continue;
        }
        Client* existing = sEntityList.FindClientByCharID(entry.first);
        if (existing == nullptr) {
            continue;
        }
        OnCharNowInStation existingPresence;
        existingPresence.charID = existing->GetCharacterID();
        existingPresence.corpID = existing->GetCorporationID();
        existingPresence.allianceID = existing->GetAllianceID();
        existingPresence.warFactionID = existing->GetWarFactionID();
        PyTuple* payload = existingPresence.Encode();
        client->SendNotification("OnCharNowInStation", "stationid", &payload);
        sLog.Green("CQ", "[CQ] Join: sent reciprocal OnCharNowInStation to joinedChar=%u about existingChar=%u",
            state.characterID, entry.first);

        // CQ protocol replay: also send an explicit avatar transform event for
        // each incumbent to the newly joined client. The 9-tuple mirrors the
        // shape used for live movement updates so the modded sharedCQClient
        // can spawn remote avatars without follow-up identity RPCs.
        PyTuple* cqReplay = new PyTuple(9);
        cqReplay->SetItem(0, new PyInt(existing->GetCharacterID()));
        cqReplay->SetItem(1, new PyFloat(entry.second.position.x));
        cqReplay->SetItem(2, new PyFloat(entry.second.position.y));
        cqReplay->SetItem(3, new PyFloat(entry.second.position.z));
        cqReplay->SetItem(4, new PyInt(worldSpaceID));
        cqReplay->SetItem(5, new PyInt(entry.second.corporationID));
        cqReplay->SetItem(6, new PyInt(entry.second.genderID));
        cqReplay->SetItem(7, new PyInt(entry.second.bloodlineID));
        cqReplay->SetItem(8, new PyInt(entry.second.raceID));
        client->SendNotification("OnCQAvatarMove", "charid", &cqReplay, false);
        sLog.Green("CQ", "[CQ] Join: replay OnCQAvatarMove to joinedChar=%u about existingChar=%u worldSpace=%u pos=(%.3f,%.3f,%.3f)",
            state.characterID, existing->GetCharacterID(), worldSpaceID,
            entry.second.position.x, entry.second.position.y, entry.second.position.z);
    }

    // Announce a newly joined avatar immediately so already-present clients
    // can spawn it without waiting for movement updates.
    BroadcastTransform(worldSpaceID, state.characterID, state.position);
    return worldSpaceID;
}

void CQManager::Leave(Client* client) {
    if (client == nullptr) {
        return;
    }

    const uint32 characterID = client->GetCharacterID();
    auto idx = m_playerToWorldspace.find(characterID);
    if (idx == m_playerToWorldspace.end()) {
        sLog.Green("CQ", "[CQ] Leave: char=%u (%s) had no CQ worldspace mapping (no-op)",
            characterID, client->GetName());
        return;
    }

    const uint32 worldSpaceID = idx->second;
    auto wsIt = m_worldspaces.find(worldSpaceID);
    if (wsIt != m_worldspaces.end()) {
        wsIt->second.occupants.erase(characterID);
        sLog.Green("CQ", "[CQ] Leave: char=%u (%s) worldSpace=%u occupantsNow=%zu",
            characterID, client->GetName(), worldSpaceID, wsIt->second.occupants.size());
    } else {
        sLog.Green("CQ", "[CQ] Leave: char=%u worldSpace=%u missing from m_worldspaces (DB still cleared)",
            characterID, worldSpaceID);
    }
    ServiceDB::SetCQOccupancy(worldSpaceID, characterID, false);
    m_playerToWorldspace.erase(idx);
}

void CQManager::UpdatePosition(Client* client, const GPoint& position) {
    if (client == nullptr) {
        return;
    }

    const uint32 characterID = client->GetCharacterID();
    const uint32 worldSpaceID = GetPlayerWorldSpace(characterID);
    if (worldSpaceID == 0) {
        sLog.Green("CQ", "[CQ] UpdatePosition: char=%u (%s) not in any worldspace (ignored)",
            characterID, client->GetName());
        return;
    }

    auto wsIt = m_worldspaces.find(worldSpaceID);
    if (wsIt == m_worldspaces.end()) {
        sLog.Error("CQ", "[CQ] UpdatePosition: char=%u worldSpace=%u not in m_worldspaces", characterID, worldSpaceID);
        return;
    }

    auto occIt = wsIt->second.occupants.find(characterID);
    if (occIt == wsIt->second.occupants.end()) {
        sLog.Green("CQ", "[CQ] UpdatePosition: char=%u not listed in worldSpace %u occupants (ignored)",
            characterID, worldSpaceID);
        return;
    }

    occIt->second.position = position;
    sLog.Green("CQ", "[CQ] UpdatePosition: char=%u worldSpace=%u pos=(%.3f,%.3f,%.3f)",
        characterID, worldSpaceID, position.x, position.y, position.z);
    BroadcastTransform(worldSpaceID, characterID, position);
}

PyList* CQManager::BuildSnapshot(uint32 worldSpaceID, uint32 excludeCharacterID) const {
    PyList* list = new PyList();
    auto wsIt = m_worldspaces.find(worldSpaceID);
    if (wsIt == m_worldspaces.end()) {
        sLog.Green("CQ", "[CQ] BuildSnapshot: worldSpace=%u not found (excludeChar=%u) -> empty list",
            worldSpaceID, excludeCharacterID);
        return list;
    }

    for (const auto& entry : wsIt->second.occupants) {
        if (entry.first == excludeCharacterID) {
            continue;
        }

        const auto& occupant = entry.second;
        PyTuple* row = new PyTuple(9);
        row->SetItem(0, new PyInt(occupant.characterID));
        row->SetItem(1, new PyFloat(occupant.position.x));
        row->SetItem(2, new PyFloat(occupant.position.y));
        row->SetItem(3, new PyFloat(occupant.position.z));
        row->SetItem(4, new PyInt(occupant.worldSpaceID));
        row->SetItem(5, new PyInt(occupant.corporationID));
        row->SetItem(6, new PyInt(occupant.genderID));
        row->SetItem(7, new PyInt(occupant.bloodlineID));
        row->SetItem(8, new PyInt(occupant.raceID));
        list->AddItem(row);
    }
    sLog.Green("CQ", "[CQ] BuildSnapshot: worldSpace=%u excludeChar=%u rows=%zu (others in room)",
        worldSpaceID, excludeCharacterID, list->size());
    return list;
}

void CQManager::BroadcastTransform(uint32 worldSpaceID, uint32 fromCharacterID, const GPoint& position) {
    auto wsIt = m_worldspaces.find(worldSpaceID);
    if (wsIt == m_worldspaces.end()) {
        sLog.Error("CQ", "[CQ] BroadcastTransform: worldSpace=%u not found (fromChar=%u)", worldSpaceID, fromCharacterID);
        return;
    }

    auto fromIt = wsIt->second.occupants.find(fromCharacterID);
    uint32 corpID = 0, bloodlineID = 0, raceID = 0;
    uint32 genderID = 0;
    if (fromIt != wsIt->second.occupants.end()) {
        corpID = fromIt->second.corporationID;
        bloodlineID = fromIt->second.bloodlineID;
        raceID = fromIt->second.raceID;
        genderID = fromIt->second.genderID;
    } else {
        sLog.Green("CQ", "[CQ] BroadcastTransform: fromChar=%u not in occupants of worldSpace=%u (identity defaults to 0)",
            fromCharacterID, worldSpaceID);
    }

    PyTuple* payload = new PyTuple(9);
    payload->SetItem(0, new PyInt(fromCharacterID));
    payload->SetItem(1, new PyFloat(position.x));
    payload->SetItem(2, new PyFloat(position.y));
    payload->SetItem(3, new PyFloat(position.z));
    payload->SetItem(4, new PyInt(worldSpaceID));
    payload->SetItem(5, new PyInt(corpID));
    payload->SetItem(6, new PyInt(genderID));
    payload->SetItem(7, new PyInt(bloodlineID));
    payload->SetItem(8, new PyInt(raceID));

    uint32 sent = 0;
    for (const auto& entry : wsIt->second.occupants) {
        if (entry.first == fromCharacterID) {
            continue;
        }
        Client* target = sEntityList.FindClientByCharID(entry.first);
        if (target == nullptr) {
            sLog.Green("CQ", "[CQ] BroadcastTransform: no online Client for charID %u (skip OnCQAvatarMove)", entry.first);
            continue;
        }
        PyIncRef(payload);
        sLog.Green("CQ", "[CQ] BroadcastTransform: OnCQAvatarMove -> char=%u (%s) fromChar=%u worldSpace=%u pos=(%.3f,%.3f,%.3f)",
            entry.first, target->GetName(), fromCharacterID, worldSpaceID, position.x, position.y, position.z);
        target->SendNotification("OnCQAvatarMove", "charid", &payload, false);
        ++sent;
    }
    sLog.Green("CQ", "[CQ] BroadcastTransform: done fromChar=%u worldSpace=%u recipients=%u", fromCharacterID, worldSpaceID, sent);
    PyDecRef(payload);
}

bool CQManager::IsInWorldSpace(uint32 worldSpaceID) const {
    return m_worldspaces.find(worldSpaceID) != m_worldspaces.end();
}

uint32 CQManager::GetPlayerWorldSpace(uint32 characterID) const {
    auto it = m_playerToWorldspace.find(characterID);
    if (it == m_playerToWorldspace.end()) {
        return 0;
    }
    return it->second;
}

uint32 CQManager::GetStationForWorldSpace(uint32 worldSpaceID) const {
    auto it = m_worldspaces.find(worldSpaceID);
    if (it == m_worldspaces.end()) {
        return 0;
    }
    return it->second.stationID;
}
