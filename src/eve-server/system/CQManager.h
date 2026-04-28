/*
 * Shared Captain's Quarters runtime manager.
 */

#ifndef __EVE_SYSTEM_CQ_MANAGER_H_
#define __EVE_SYSTEM_CQ_MANAGER_H_

#include "eve-server.h"
#include "ServiceDB.h"

class Client;

struct CQOccupantState {
    uint32 characterID{0};
    uint32 stationID{0};
    uint32 worldSpaceID{0};
    GPoint position{};
    float  yaw{0.0f};
    uint32 corporationID{0};
    uint32 bloodlineID{0};
    uint32 raceID{0};
    uint8  genderID{0};
    // Phase 2f: ActionObject occupancy. actionObjectUID identifies the
    // furniture (couch/chair/etc) via cfg.actionObjects -- stable across
    // clients so both sides can map it back to a local scene entity.
    // actionStationIdx selects the seat within that object (0..N-1).
    // (0, -1) means "not using any action object" (standing).
    uint32 actionObjectUID{0};
    int32  actionStationIdx{-1};
};

class CQManager : public Singleton<CQManager> {
public:
    CQManager() = default;
    ~CQManager() = default;

    uint32 EnsureWorldSpaceForStation(uint32 stationID);
    uint32 Join(Client* client, uint32 stationID);
    void Leave(Client* client);
    void UpdateTransform(Client* client, const GPoint& position, float yaw);
    void SetActionState(Client* client, uint32 actionObjectUID, int32 actionStationIdx);
    PyList* BuildSnapshot(uint32 worldSpaceID, uint32 excludeCharacterID = 0) const;
    void BroadcastTransform(uint32 worldSpaceID, uint32 fromCharacterID, const GPoint& position, float yaw);
    void BroadcastAction(uint32 worldSpaceID, uint32 fromCharacterID, uint32 actionObjectUID, int32 actionStationIdx);
    /// Ephemeral emote (morpheme index 0..kMaxBroadcastEmote-1, client-mapped to BroadcastRequest name).
    void BroadcastEmote(uint32 worldSpaceID, uint32 fromCharacterID, int32 emoteIndex);
    bool IsInWorldSpace(uint32 worldSpaceID) const;
    uint32 GetPlayerWorldSpace(uint32 characterID) const;
    uint32 GetStationForWorldSpace(uint32 worldSpaceID) const;

    /// OnCQCustomAgentGone: (instanceCharID,) — for debug/admin delete (optional client handler).
    void BroadcastCustomAgentGone(uint32 worldSpaceID, uint32 instanceCharID);
    void BroadcastCustomAgentMove(uint32 worldSpaceID, const CQCustomAgentRow& ag);
    const CQOccupantState* GetOccupantStateForChar(uint32 characterID) const;

private:
    void AppendCustomAgentSnapshot(uint32 stationID, uint32 worldSpaceID, PyList* list) const;
    struct CQWorldSpace {
        uint32 worldSpaceID{0};
        uint32 stationID{0};
        std::map<uint32, CQOccupantState> occupants;
    };

    std::map<uint32, CQWorldSpace> m_worldspaces;
    std::map<uint32, uint32> m_playerToWorldspace;
};

#define sCQMgr (CQManager::get())

#endif
