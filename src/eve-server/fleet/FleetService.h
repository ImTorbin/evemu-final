
 /**
  * @name FleetService.h
  *     Fleet Service code for EVEmu
  *
  * @Author:        Allan
  * @date:          05 August 2014 (original skeleton outline)
  * @update:        21 November 2017 (begin actual implementation)
  *
  */

#ifndef EVEMU_SRC_FLEET_SVC_H_
#define EVEMU_SRC_FLEET_SVC_H_

#include "eve-common.h"
#include "utils/Singleton.h"

#include "Client.h"
#include "EntityList.h"
#include "packets/Fleet.h"
#include "fleet/FleetData.h"

#include <unordered_map>

/** Active ship-lit cyno beacon for fleet UI / jump portals (not persisted). */
struct FleetBeaconEntry {
    uint32 beaconItemID {0};
    uint32 solarSystemID {0};
    bool covert {false};
};

/** Active titan / Black Ops jump portal bridge session. */
struct FleetBridgeEntry {
    uint32 bridgeShipID {0};
    uint32 beaconItemID {0};
    uint32 solarSystemID {0};
    bool covert {false};
    double remainingMassKg {0};
    /** Per-jump cost multiplier (from module AttrJumpPortalConsumptionMassFactor). */
    double massCostFactor {1.0};
    /** Expiry from utils_time GetSteadyTime() (milliseconds). */
    int64 expirySteadyMs {0};
};

class FleetService
: public Singleton<FleetService>
{
public:
    FleetService();
    ~FleetService()                              { /* do nothing here */ }

    void Initialize(EVEServiceManager& svc);

    uint32 CreateFleet(Client* pClient);
    PyRep* CreateWing(uint32 fleetID);
    void CreateSquad(uint32 fleetID, uint32 wingID);
    void DeleteFleet(uint32 fleetID);
    void DeleteWing(uint32 wingID);
    void DeleteSquad(uint32 squadID);

    PyRep* GetAvailableFleets();
    PyRep* GetWings(uint32 fleetID);

    PyRep* GetFleetAdvert(uint32 fleetID);
    void CreateFleetAdvert(uint32 fleetID, FleetAdvert data);
    void RemoveFleetAdvert(uint32 fleetID);

    void GetWingIDs(uint32 fleetID, std::vector< uint32 >& wingIDs);
    void GetSquadIDs(uint32 wingID, std::vector< uint32 >& squadIDs);

    void GetFleetData(uint32 fleetID, FleetData& data);
    void GetWingData(uint32 wingID, WingData& data);
    void GetSquadData(uint32 squadID, SquadData& data);
    void GetMemeberVec(uint32 fleetID, std::vector<Client*>& data);

    bool IsWingActive(int32 wingID);

    void SetMOTD(uint32 fleetID, std::string motd);
    PyRep* GetMOTD(uint32 fleetID);

    void RenameWing(uint32 wingID, std::string name);
    void RenameSquad(uint32 squadID, std::string name);

    void UpdateBoost(uint32 fleetID, bool fleet, std::list< int32 >& wing, std::list< int32 >& squad);
    void UpdateOptions(uint32 fleetID, bool isFreeMove, bool isRegistered, bool isVoiceEnabled);

    bool AddMember(Client* pClient, uint32 fleetID, int32 wingID, int32 squadID, int8 job, int8 role, int8 booster);
    bool UpdateMember(uint32 charID, uint32 fleetID, int32 newWingID, int32 newSquadID, int8 newJob, int8 newRole, int8 newBooster);

    Client* GetFleetLeader(uint32 fleetID);
    uint32 GetFleetLeaderID(uint32 fleetID);
    Client* GetWingLeader(uint32 wingID);
    uint32 GetWingLeaderID(uint32 wingID);
    Client* GetSquadLeader(uint32 squadID);
    uint32 GetSquadLeaderID(uint32 squadID);

    std::string GetFleetName(uint32 fleetID);
    std::string GetWingName(uint32 wingID);
    std::string GetSquadName(uint32 squadID);

    std::string GetFleetDescription(uint32 fleetID);

    void GetRandUnitIDs(uint32 fleetID, int32& wingID, int32& squadID);

    void LeaveFleet(Client* pClient);

    void FleetBroadcast(Client* pFrom, uint32 itemID, int8 scope, int8 group, std::string msg);

    void SendActiveStatus(uint32 fleetID, int32 wingID, int32 squadID);
    // this sends "*fleetid" update to all fleet memebers and is sequenced
    void SendFleetUpdate(uint32 fleetID, const char *notifyType, PyTuple *payload);

    std::string GetBoosterData(uint32 fleetID, uint16& length);

    bool GetInviteData(uint32 charID, InviteData& data);
    bool SaveInviteData(uint32 charID, InviteData& data);
    void RemoveInviteData(uint32 charID);

    bool AddJoinRequest(uint32 fleetID, Client* pClient);
    void GetJoinRequests(uint32 fleetID, std::vector<Client*>& data);
    void RemoveJoinRequest(uint32 fleetID, Client* pClient);

    uint8 GetFleetMemberCount(uint32 fleetID)          { return m_fleetMembers.count(fleetID); }

    std::string GetJobName(int8 job);
    std::string GetRoleName(int8 role);
    std::string GetBoosterName(int8 booster);

    std::string GetBCastScopeName(int8 scope);
    std::string GetBCastGroupName(int8 group);

    void GetFleetMembersOnGrid(Client* pClient, std::vector<uint32>& data);
    void GetFleetMembersInSystem(Client* pClient, std::vector<uint32>& data);
    void GetFleetClientsInSystem(Client* pClient, std::vector<Client*>& data);
    std::vector<Client *> GetFleetClients(uint32 fleetID);

    /* Returns fleet clients the issuer is allowed to fleet-warp:
     *   FleetLeader  = every member of the fleet,
     *   WingLeader   = every member of the issuer's wing,
     *   SquadLeader  = every member of the issuer's squad.
     * Filters by same system as the issuer and excludes the issuer itself.
     * Returns false (and leaves out empty) if the issuer has no authority
     * (Member role or not actually in a fleet).
     */
    bool GetFleetWarpTargets(Client* issuer, std::vector<Client*>& out);

    void RegisterActiveBeacon(uint32 charID, uint32 beaconItemID, uint32 solarSystemID, bool covert);
    void UnregisterActiveBeacon(uint32 charID);
    bool GetActiveBeaconForChar(uint32 charID, FleetBeaconEntry& out) const;

    /** First matching beacon held by any member of the fleet (same covert flag). */
    bool FindCompatibleFleetBeacon(uint32 fleetID, bool requireCovert, FleetBeaconEntry& out) const;

    void RegisterJumpPortal(uint32 bridgeShipID, uint32 beaconItemID, uint32 solarSystemID,
                            bool covert, double totalMassKg, double massCostFactor, int64 expirySteadyMs);
    void UnregisterJumpPortal(uint32 bridgeShipID);
    bool GetActiveBridgeForShip(uint32 bridgeShipID, FleetBridgeEntry& out) const;
    bool ConsumeJumpPortalMass(uint32 bridgeShipID, double shipMassKg);
    void InvalidateBridgesToBeacon(uint32 beaconItemID);

protected:
    void RemoveMember(Client* pClient);

    void IncFleetSquads(uint32 fleetID, uint32 wingID);
    void DecFleetSquads(uint32 fleetID, uint32 wingID);

    void SetWingBoostData(uint32 wingID, BoostData& bData);
    void SetSquadBoostData(uint32 squadID, BoostData bData, bool& sboost);

    uint32 m_fleetID;
    uint32 m_wingID;
    uint32 m_squadID;

private:
    LSCService* m_lsc;
    bool m_initalized;

    std::map<uint32, InviteData>        m_inviteData;       // charID/data

    std::map<uint32, FleetAdvert>       m_fleetAdvertMap;   // fleetID/data

    std::map<uint32, FleetData>         m_fleetDataMap;     // fleetID/data
    std::map<uint32, WingData>          m_wingDataMap;      // wingID/data
    std::map<uint32, SquadData>         m_squadDataMap;     // squadID/data

    std::multimap<uint32, Client*>      m_joinReq;          // fleetID/Client*
    std::multimap<uint32, Client*>      m_fleetMembers;     // fleetID/Client*
    std::multimap<uint32, uint32>       m_fleetWings;       // fleetID/wingIDs
    std::multimap<uint32, uint32>       m_wingSquads;       // wingID/squadIDs

    std::unordered_map<uint32, FleetBeaconEntry> m_activeBeaconByChar;
    std::unordered_map<uint32, FleetBridgeEntry> m_activeBridgeByShip;
};

//Singleton
#define sFltSvc \
    ( FleetService::get() )

#endif  // EVEMU_SRC_FLEET_SVC_H_
