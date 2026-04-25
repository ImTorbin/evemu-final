/*
 * Shared Captain's Quarters runtime manager.
 */

#ifndef __EVE_SYSTEM_CQ_MANAGER_H_
#define __EVE_SYSTEM_CQ_MANAGER_H_

#include "eve-server.h"

class Client;

struct CQOccupantState {
    uint32 characterID{0};
    uint32 stationID{0};
    uint32 worldSpaceID{0};
    GPoint position{};
    uint32 corporationID{0};
    uint32 bloodlineID{0};
    uint32 raceID{0};
    uint8  genderID{0};
};

class CQManager : public Singleton<CQManager> {
public:
    CQManager() = default;
    ~CQManager() = default;

    uint32 EnsureWorldSpaceForStation(uint32 stationID);
    uint32 Join(Client* client, uint32 stationID);
    void Leave(Client* client);
    void UpdatePosition(Client* client, const GPoint& position);
    PyList* BuildSnapshot(uint32 worldSpaceID, uint32 excludeCharacterID = 0) const;
    void BroadcastTransform(uint32 worldSpaceID, uint32 fromCharacterID, const GPoint& position);
    bool IsInWorldSpace(uint32 worldSpaceID) const;
    uint32 GetPlayerWorldSpace(uint32 characterID) const;
    uint32 GetStationForWorldSpace(uint32 worldSpaceID) const;

private:
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
