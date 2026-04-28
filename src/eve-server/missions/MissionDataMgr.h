
 /**
  * @name MissionDataMgr.h
  *   memory object caching system for managing and saving ingame data specific to missions
  *
  * @Author:        Allan
  * @date:      24 June 2018
  *
  */


#ifndef _EVE_SERVER_MISSION_DATAMANAGER_H__
#define _EVE_SERVER_MISSION_DATAMANAGER_H__

#include <vector>

#include "../eve-server.h"
#include "missions/MissionDB.h"


class MissionDataMgr
: public Singleton< MissionDataMgr >
{
public:
    MissionDataMgr();
    ~MissionDataMgr();

    int                 Initialize();

    void                Clear();
    void                Close()                         { Clear(); }
    void                GetInfo();

    void                Process();

    void                AddMissionOffer(uint32 charID, MissionOffer& data);
    void                UpdateMissionData(uint32 charID, MissionOffer& data);
    void                RemoveMissionOffer(uint32 charID, MissionOffer& data);
    void                LoadMissionOffers(uint32 charID, std::vector<MissionOffer>& data);
    void                LoadAgentOffers(const uint32 agentID, std::map<uint32, MissionOffer>& data);
    void                CreateMissionOffer(uint8 typeID, uint8 level, uint8 raceID, bool important, uint32 agentID, uint32 characterID, MissionOffer& data, uint32 agentCorpID = 0);
    /** Random among mission kinds allowed for this agent division; mining agents use mining only when templates exist. */
    uint8               ChooseMissionOfferKind(uint8 level, uint8 raceID, bool important, uint32 agentID, uint32 characterID, uint32 agentCorpID, uint8 divisionID);

    std::string         GetTypeName(uint8 typeID);
    std::string         GetTypeLabel(uint8 typeID);
    /** Journal mission-type label; custom per agent without changing qstCourier.typeID. */
    std::string         GetTypeLabelForAgent(uint32 agentID, uint8 typeID);

    PyString* GetKillRes()                               { PyIncRef(KillPNG); return KillPNG; }
    PyString* GetMiningRes()                             { PyIncRef(MiningPNG); return MiningPNG; }
    PyString* GetCourierRes()                            { PyIncRef(CourierPNG); return CourierPNG; }

protected:
    void                Populate();

private:
    bool                BuildCourierTemplateVector(uint8 level, uint8 raceID, bool important, uint32 agentID, uint32 characterID, uint32 agentCorpID, std::vector<CourierData>& cVec);
    bool                HasMiningTemplates(uint8 level, bool important) const;
    bool                HasEncounterTemplates(uint8 level, bool important, uint32 agentCorpID) const;

    uint8 m_procCount;

    std::map<std::string, uint32> m_names;
    std::multimap<uint8, CourierData> m_courier;    // level/data
    std::multimap<uint8, CourierData> m_courierImp;    // level/data
    std::multimap<uint32, CourierData> m_courierByAgent;  // agtAgents.agentID / courier row for that agent only
    std::multimap<uint8, CourierData> m_mining;     // level/data
    std::multimap<uint8, CourierData> m_miningImp;     // level/data
    std::multimap<uint8, MissionData> m_missions;   // level/data
    std::multimap<uint8, MissionData> m_missionsImp;   // level/data
    std::multimap<uint32, MissionOffer> m_offers;   // charID/data      current mission offers by charID
    std::multimap<uint32, MissionOffer> m_aoffers;   // agentID/data    current mission offers by agentID
    std::multimap<uint32, MissionOffer> m_xoffers;   // charID/data     expired/completed offers by charID

    //  mission png resources...
    PyString* CourierPNG;
    PyString* MiningPNG;
    PyString* KillPNG;

};

//Singleton
#define sMissionDataMgr \
( MissionDataMgr::get() )


#endif  // _EVE_SERVER_MISSION_DATAMANAGER_H__
