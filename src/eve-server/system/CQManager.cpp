#include "eve-server.h"

#include "Client.h"
#include "ServiceDB.h"
#include "system/CQManager.h"

namespace {
/// World-space yaw in DB is often sampled in DCC with +Z (or a prop axis) as "forward", while
/// the CQ avatar's Trinity mesh faces the opposite heading for the same scalar. For mission-
/// linked custom agents, add 180 deg so the character faces into the room; adjust DB if needed.
static float CQYawForClientFromMissionCustomAgent(const CQCustomAgentRow& ag) {
    float y = ag.yaw;
    if (ag.missionAgentID == 0) {
        return y;
    }
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTwoPi = 6.28318530717958647692f;
    y += kPi;
    while (y > kPi) {
        y -= kTwoPi;
    }
    while (y <= -kPi) {
        y += kTwoPi;
    }
    return y;
}
uint32 CQProtocolCharId(const CQCustomAgentRow& ag) {
    return (ag.missionAgentID != 0) ? ag.missionAgentID : ag.instanceCharID;
}
/// Optional paper-doll override when mission agent uses a different template char.
uint32 CQAppearanceOverride(const CQCustomAgentRow& ag) {
    if (ag.missionAgentID == 0) {
        return ag.appearanceCharID;
    }
    if (ag.appearanceCharID != 0 && ag.appearanceCharID != ag.missionAgentID) {
        return ag.appearanceCharID;
    }
    return 0;
}
/// OnCQAvatarMove: (0 charID, 1-3 pos, 4 worldSpace, 5 corp, 6 gender, 7 blood, 8 race, 9 yaw, 10 appearanceOverride for paper doll: 0 = use charID)
PyTuple* EncodeCQAvatarMove11(uint32 fromCharacterID, const GPoint& position, uint32 worldSpaceID, uint32 corpID, uint32 genderID, uint32 bloodlineID, uint32 raceID, float yaw, uint32 appearanceOverride) {
    PyTuple* payload = new PyTuple(11);
    payload->SetItem(0, new PyInt(fromCharacterID));
    payload->SetItem(1, new PyFloat(position.x));
    payload->SetItem(2, new PyFloat(position.y));
    payload->SetItem(3, new PyFloat(position.z));
    payload->SetItem(4, new PyInt(worldSpaceID));
    payload->SetItem(5, new PyInt(corpID));
    payload->SetItem(6, new PyInt(genderID));
    payload->SetItem(7, new PyInt(bloodlineID));
    payload->SetItem(8, new PyInt(raceID));
    payload->SetItem(9, new PyFloat(yaw));
    payload->SetItem(10, new PyInt(appearanceOverride));
    return payload;
}
} // namespace

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

    bool cqLayoutChanged = false;
    ServiceDB::EnsureCQMissionAgentSpawnsForStation(stationID, &cqLayoutChanged);
    if (cqLayoutChanged) {
        auto wsIt = m_worldspaces.find(worldSpaceID);
        if (wsIt != m_worldspaces.end() && !wsIt->second.occupants.empty()) {
            std::vector<CQCustomAgentRow> agents;
            ServiceDB::GetCQCustomAgentsForStation(stationID, agents);
            for (const auto& ag : agents) {
                BroadcastCustomAgentMove(worldSpaceID, ag);
            }
            sLog.Green("CQ", "[CQ] EnsureWorldSpaceForStation: pushed %zu custom/mission CQ agents to occupants (station=%u)",
                agents.size(), stationID);
        }
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
    state.yaw = 0.0f;
    state.actionObjectUID = 0;
    state.actionStationIdx = -1;
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
        // each incumbent to the newly joined client. 11-tuple: index 9 = yaw;
        // index 10 = appearanceOverride (0 = use charID for paper doll).
        PyTuple* cqReplay = EncodeCQAvatarMove11(
            existing->GetCharacterID(), entry.second.position, worldSpaceID,
            entry.second.corporationID, entry.second.genderID, entry.second.bloodlineID, entry.second.raceID,
            entry.second.yaw, 0u);
        client->SendNotification("OnCQAvatarMove", "charid", &cqReplay, false);
        sLog.Green("CQ", "[CQ] Join: replay OnCQAvatarMove to joinedChar=%u about existingChar=%u worldSpace=%u pos=(%.3f,%.3f,%.3f) yaw=%.3f",
            state.characterID, existing->GetCharacterID(), worldSpaceID,
            entry.second.position.x, entry.second.position.y, entry.second.position.z,
            entry.second.yaw);

        // Phase 2f: if the incumbent is currently using an ActionObject (e.g.
        // sitting on a couch seat), tell the late joiner so it can snap them
        // straight to the correct cushion. Skip the replay when nobody is
        // sitting -- standing is the implicit default on the receiver.
        if (entry.second.actionStationIdx >= 0) {
            PyTuple* actReplay = new PyTuple(3);
            actReplay->SetItem(0, new PyInt(existing->GetCharacterID()));
            actReplay->SetItem(1, new PyInt(entry.second.actionObjectUID));
            actReplay->SetItem(2, new PyInt(entry.second.actionStationIdx));
            client->SendNotification("OnCQAvatarAction", "charid", &actReplay, false);
            sLog.Green("CQ", "[CQ] Join: replay OnCQAvatarAction to joinedChar=%u about existingChar=%u aoUID=%u stationIdx=%d",
                state.characterID, existing->GetCharacterID(),
                entry.second.actionObjectUID, entry.second.actionStationIdx);
        }
    }

    // Custom agents are in BuildSnapshot / GetSnapshot (no OnCQAvatarMove replay
    // here: would duplicate snapshot-driven spawns on the client).

    // Announce a newly joined avatar immediately so already-present clients
    // can spawn it without waiting for movement updates.
    BroadcastTransform(worldSpaceID, state.characterID, state.position, state.yaw);
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

void CQManager::UpdateTransform(Client* client, const GPoint& position, float yaw) {
    if (client == nullptr) {
        return;
    }

    const uint32 characterID = client->GetCharacterID();
    const uint32 worldSpaceID = GetPlayerWorldSpace(characterID);
    if (worldSpaceID == 0) {
        sLog.Green("CQ", "[CQ] UpdateTransform: char=%u (%s) not in any worldspace (ignored)",
            characterID, client->GetName());
        return;
    }

    auto wsIt = m_worldspaces.find(worldSpaceID);
    if (wsIt == m_worldspaces.end()) {
        sLog.Error("CQ", "[CQ] UpdateTransform: char=%u worldSpace=%u not in m_worldspaces", characterID, worldSpaceID);
        return;
    }

    auto occIt = wsIt->second.occupants.find(characterID);
    if (occIt == wsIt->second.occupants.end()) {
        sLog.Green("CQ", "[CQ] UpdateTransform: char=%u not listed in worldSpace %u occupants (ignored)",
            characterID, worldSpaceID);
        return;
    }

    occIt->second.position = position;
    occIt->second.yaw = yaw;
    sLog.Green("CQ", "[CQ] UpdateTransform: char=%u worldSpace=%u pos=(%.3f,%.3f,%.3f) yaw=%.3f",
        characterID, worldSpaceID, position.x, position.y, position.z, yaw);
    // While seated (couch etc.), the client still streams root motion (approach
    // path, in-between animation). Broadcasting that as OnCQAvatarMove confuses
    // observers' receive-side: it races with / overrides OnCQAvatarAction and
    // the local seat/cushion snap. Peers get pose from OnCQAvatarAction only.
    if (occIt->second.actionStationIdx < 0) {
        BroadcastTransform(worldSpaceID, characterID, position, yaw);
    }
}

void CQManager::SetActionState(Client* client, uint32 actionObjectUID, int32 actionStationIdx) {
    if (client == nullptr) {
        return;
    }

    const uint32 characterID = client->GetCharacterID();
    const uint32 worldSpaceID = GetPlayerWorldSpace(characterID);
    if (worldSpaceID == 0) {
        sLog.Green("CQ", "[CQ] SetActionState: char=%u (%s) not in any worldspace (ignored)",
            characterID, client->GetName());
        return;
    }

    auto wsIt = m_worldspaces.find(worldSpaceID);
    if (wsIt == m_worldspaces.end()) {
        sLog.Error("CQ", "[CQ] SetActionState: char=%u worldSpace=%u not in m_worldspaces", characterID, worldSpaceID);
        return;
    }

    auto occIt = wsIt->second.occupants.find(characterID);
    if (occIt == wsIt->second.occupants.end()) {
        sLog.Green("CQ", "[CQ] SetActionState: char=%u not listed in worldSpace %u occupants (ignored)",
            characterID, worldSpaceID);
        return;
    }

    // Idempotency: if the state hasn't actually changed, skip the broadcast
    // so the action poll tick can hammer this RPC without flooding peers.
    if (occIt->second.actionObjectUID == actionObjectUID && occIt->second.actionStationIdx == actionStationIdx) {
        return;
    }

    const int32 oldStationIdx = occIt->second.actionStationIdx;
    occIt->second.actionObjectUID = actionObjectUID;
    occIt->second.actionStationIdx = actionStationIdx;
    sLog.Green("CQ", "[CQ] SetActionState: char=%u worldSpace=%u aoUID=%u stationIdx=%d",
        characterID, worldSpaceID, actionObjectUID, actionStationIdx);
    BroadcastAction(worldSpaceID, characterID, actionObjectUID, actionStationIdx);
    // Standing up: observers' receive-side was starved of OnCQAvatarMove while
    // the pilot was seated (UpdateTransform does not broadcast). Re-send the
    // last authoritative position+yaw with the stand notify so the remote
    // can align root to floor before the next 10Hz transform sample.
    if (actionStationIdx < 0 && oldStationIdx >= 0) {
        BroadcastTransform(worldSpaceID, characterID, occIt->second.position, occIt->second.yaw);
    }
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
        // Phase 2f+ : 13-tuple. Indices 10/11 = action; 12 = appearanceOverride (0 = use charID for doll).
        PyTuple* row = new PyTuple(13);
        row->SetItem(0, new PyInt(occupant.characterID));
        row->SetItem(1, new PyFloat(occupant.position.x));
        row->SetItem(2, new PyFloat(occupant.position.y));
        row->SetItem(3, new PyFloat(occupant.position.z));
        row->SetItem(4, new PyInt(occupant.worldSpaceID));
        row->SetItem(5, new PyInt(occupant.corporationID));
        row->SetItem(6, new PyInt(occupant.genderID));
        row->SetItem(7, new PyInt(occupant.bloodlineID));
        row->SetItem(8, new PyInt(occupant.raceID));
        row->SetItem(9, new PyFloat(occupant.yaw));
        row->SetItem(10, new PyInt(occupant.actionObjectUID));
        row->SetItem(11, new PyInt(occupant.actionStationIdx));
        row->SetItem(12, new PyInt(0));
        list->AddItem(row);
    }
    const uint32 stationID = GetStationForWorldSpace(worldSpaceID);
    if (stationID != 0) {
        AppendCustomAgentSnapshot(stationID, worldSpaceID, list);
    }
    sLog.Green("CQ", "[CQ] BuildSnapshot: worldSpace=%u excludeChar=%u rows=%zu (others in room + custom)",
        worldSpaceID, excludeCharacterID, list->size());
    return list;
}

void CQManager::BroadcastTransform(uint32 worldSpaceID, uint32 fromCharacterID, const GPoint& position, float yaw) {
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

    PyTuple* payload = EncodeCQAvatarMove11(fromCharacterID, position, worldSpaceID, corpID, genderID, bloodlineID, raceID, yaw, 0u);

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
        sLog.Green("CQ", "[CQ] BroadcastTransform: OnCQAvatarMove -> char=%u (%s) fromChar=%u worldSpace=%u pos=(%.3f,%.3f,%.3f) yaw=%.3f",
            entry.first, target->GetName(), fromCharacterID, worldSpaceID, position.x, position.y, position.z, yaw);
        target->SendNotification("OnCQAvatarMove", "charid", &payload, false);
        ++sent;
    }
    sLog.Green("CQ", "[CQ] BroadcastTransform: done fromChar=%u worldSpace=%u recipients=%u", fromCharacterID, worldSpaceID, sent);
    PyDecRef(payload);
}

void CQManager::BroadcastAction(uint32 worldSpaceID, uint32 fromCharacterID, uint32 actionObjectUID, int32 actionStationIdx) {
    auto wsIt = m_worldspaces.find(worldSpaceID);
    if (wsIt == m_worldspaces.end()) {
        sLog.Error("CQ", "[CQ] BroadcastAction: worldSpace=%u not found (fromChar=%u)", worldSpaceID, fromCharacterID);
        return;
    }

    PyTuple* payload = new PyTuple(3);
    payload->SetItem(0, new PyInt(fromCharacterID));
    payload->SetItem(1, new PyInt(actionObjectUID));
    payload->SetItem(2, new PyInt(actionStationIdx));

    uint32 sent = 0;
    for (const auto& entry : wsIt->second.occupants) {
        if (entry.first == fromCharacterID) {
            continue;
        }
        Client* target = sEntityList.FindClientByCharID(entry.first);
        if (target == nullptr) {
            sLog.Green("CQ", "[CQ] BroadcastAction: no online Client for charID %u (skip OnCQAvatarAction)", entry.first);
            continue;
        }
        PyIncRef(payload);
        sLog.Green("CQ", "[CQ] BroadcastAction: OnCQAvatarAction -> char=%u (%s) fromChar=%u aoUID=%u stationIdx=%d",
            entry.first, target->GetName(), fromCharacterID, actionObjectUID, actionStationIdx);
        target->SendNotification("OnCQAvatarAction", "charid", &payload, false);
        ++sent;
    }
    sLog.Green("CQ", "[CQ] BroadcastAction: done fromChar=%u worldSpace=%u recipients=%u", fromCharacterID, worldSpaceID, sent);
    PyDecRef(payload);
}

void CQManager::BroadcastEmote(uint32 worldSpaceID, uint32 fromCharacterID, int32 emoteIndex) {
    auto wsIt = m_worldspaces.find(worldSpaceID);
    if (wsIt == m_worldspaces.end()) {
        sLog.Error("CQ", "[CQ] BroadcastEmote: worldSpace=%u not found (fromChar=%u)", worldSpaceID, fromCharacterID);
        return;
    }

    PyTuple* payload = new PyTuple(2);
    payload->SetItem(0, new PyInt(fromCharacterID));
    payload->SetItem(1, new PyInt(emoteIndex));

    uint32 sent = 0;
    for (const auto& entry : wsIt->second.occupants) {
        if (entry.first == fromCharacterID) {
            continue;
        }
        Client* target = sEntityList.FindClientByCharID(entry.first);
        if (target == nullptr) {
            sLog.Green("CQ", "[CQ] BroadcastEmote: no online Client for charID %u (skip OnCQAvatarEmote)", entry.first);
            continue;
        }
        PyIncRef(payload);
        sLog.Green("CQ", "[CQ] BroadcastEmote: OnCQAvatarEmote -> char=%u (%s) fromChar=%u idx=%d",
            entry.first, target->GetName(), fromCharacterID, emoteIndex);
        target->SendNotification("OnCQAvatarEmote", "charid", &payload, false);
        ++sent;
    }
    sLog.Green("CQ", "[CQ] BroadcastEmote: done fromChar=%u worldSpace=%u recipients=%u", fromCharacterID, worldSpaceID, sent);
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

void CQManager::AppendCustomAgentSnapshot(uint32 stationID, uint32 worldSpaceID, PyList* list) const {
    std::vector<CQCustomAgentRow> agents;
    ServiceDB::GetCQCustomAgentsForStation(stationID, agents);
    for (const auto& ag : agents) {
        GPoint p;
        p.x = ag.posX;
        p.y = ag.posY;
        p.z = ag.posZ;
        PyTuple* row = new PyTuple(13);
        row->SetItem(0, new PyInt(CQProtocolCharId(ag)));
        row->SetItem(1, new PyFloat(p.x));
        row->SetItem(2, new PyFloat(p.y));
        row->SetItem(3, new PyFloat(p.z));
        row->SetItem(4, new PyInt(worldSpaceID));
        row->SetItem(5, new PyInt(ag.corporationID));
        row->SetItem(6, new PyInt(ag.genderID));
        row->SetItem(7, new PyInt(ag.bloodlineID));
        row->SetItem(8, new PyInt(ag.raceID));
        row->SetItem(9, new PyFloat(CQYawForClientFromMissionCustomAgent(ag)));
        row->SetItem(10, new PyInt(0));
        row->SetItem(11, new PyInt(-1));
        row->SetItem(12, new PyInt(CQAppearanceOverride(ag)));
        list->AddItem(row);
    }
}

void CQManager::BroadcastCustomAgentMove(uint32 worldSpaceID, const CQCustomAgentRow& ag) {
    auto wsIt = m_worldspaces.find(worldSpaceID);
    if (wsIt == m_worldspaces.end()) {
        sLog.Error("CQ", "[CQ] BroadcastCustomAgentMove: worldSpace=%u not found (staRow=%u)", worldSpaceID, ag.agentID);
        return;
    }
    GPoint p;
    p.x = ag.posX;
    p.y = ag.posY;
    p.z = ag.posZ;
    const uint32 pc = CQProtocolCharId(ag);
    const float yClient = CQYawForClientFromMissionCustomAgent(ag);
    PyTuple* payload = EncodeCQAvatarMove11(
        pc, p, worldSpaceID,
        ag.corporationID, ag.genderID, ag.bloodlineID, ag.raceID,
        yClient, CQAppearanceOverride(ag));

    uint32 sent = 0;
    for (const auto& entry : wsIt->second.occupants) {
        Client* target = sEntityList.FindClientByCharID(entry.first);
        if (target == nullptr) {
            continue;
        }
        PyIncRef(payload);
        target->SendNotification("OnCQAvatarMove", "charid", &payload, false);
        ++sent;
    }
    sLog.Green("CQ", "[CQ] BroadcastCustomAgentMove: protocolChar=%u worldSpace=%u recipients=%u", pc, worldSpaceID, sent);
    PyDecRef(payload);
}

void CQManager::BroadcastCustomAgentGone(uint32 worldSpaceID, uint32 instanceCharID) {
    auto wsIt = m_worldspaces.find(worldSpaceID);
    if (wsIt == m_worldspaces.end()) {
        return;
    }
    PyTuple* payload = new PyTuple(1);
    payload->SetItem(0, new PyInt(instanceCharID));
    uint32 sent = 0;
    for (const auto& entry : wsIt->second.occupants) {
        Client* target = sEntityList.FindClientByCharID(entry.first);
        if (target == nullptr) {
            continue;
        }
        PyIncRef(payload);
        target->SendNotification("OnCQCustomAgentGone", "charid", &payload, false);
        ++sent;
    }
    sLog.Green("CQ", "[CQ] BroadcastCustomAgentGone: instance=%u worldSpace=%u recipients=%u", instanceCharID, worldSpaceID, sent);
    PyDecRef(payload);
}

const CQOccupantState* CQManager::GetOccupantStateForChar(uint32 characterID) const {
    auto idx = m_playerToWorldspace.find(characterID);
    if (idx == m_playerToWorldspace.end()) {
        return nullptr;
    }
    auto wsIt = m_worldspaces.find(idx->second);
    if (wsIt == m_worldspaces.end()) {
        return nullptr;
    }
    auto occIt = wsIt->second.occupants.find(characterID);
    if (occIt == wsIt->second.occupants.end()) {
        return nullptr;
    }
    return &occIt->second;
}
