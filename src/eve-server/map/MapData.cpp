
/**
 * @name MapData.cpp
 *   a group of methods and functions to get map info.
 *     this is mostly used for getting random points in system, system jumps, and misc mission destination info
 *  - added static data for StationExtraInfo (from mapservice)
 * @Author:         Allan
 * @date:   13 November 2018
 */
#include "../StaticDataMgr.h"
#include "agents/Agent.h"
#include "../../eve-common/EVE_Defines.h"
#include "map/MapData.h"
#include "station/StationDB.h"
#include "map/MapDB.h"
#include "station/StationDataMgr.h"
#include "system/SystemManager.h"
#include "system/SystemEntity.h"

#include "../../eve-common/EVE_Map.h"
#include "../../eve-common/EVE_Agent.h"
#include "../../eve-common/EVE_Missions.h"

#include <map>
#include <queue>
#include <vector>

namespace {

    GPoint RandomAnomalyOffsetFrom(const GPoint& anchor)
    {
        GPoint pos(anchor);
        pos.MakeRandomPointOnSphereLayer(ONE_AU_IN_METERS / 3, ONE_AU_IN_METERS * 4);
        return pos;
    }

    /** Planets, then asteroid belts, then moons; avoids empty mapDenormalize / bad random index bugs. */
    GPoint AnomalyAnchorForSystem(SystemManager* pSys, uint32 systemID)
    {
        std::vector<DBGPointEntity> ents;
        uint8 n = 0;

        ents.clear();
        n = 0;
        MapDB::GetPlanets(systemID, ents, n);
        if (!ents.empty()) {
            const size_t idx = static_cast<size_t>(MakeRandomInt(0, static_cast<int64>(ents.size() - 1)));
            if (pSys != nullptr) {
                SystemEntity* pSE = pSys->GetSE(ents[idx].itemID);
                if (pSE != nullptr)
                    return RandomAnomalyOffsetFrom(pSE->GetPosition());
            }
            return RandomAnomalyOffsetFrom(ents[idx].position);
        }

        ents.clear();
        n = 0;
        MapDB::GetBelts(systemID, ents, n);
        if (!ents.empty()) {
            const size_t idx = static_cast<size_t>(MakeRandomInt(0, static_cast<int64>(ents.size() - 1)));
            return RandomAnomalyOffsetFrom(ents[idx].position);
        }

        ents.clear();
        n = 0;
        MapDB::GetMoons(systemID, ents, n);
        if (!ents.empty()) {
            const size_t idx = static_cast<size_t>(MakeRandomInt(0, static_cast<int64>(ents.size() - 1)));
            return RandomAnomalyOffsetFrom(ents[idx].position);
        }

        return RandomAnomalyOffsetFrom(NULL_ORIGIN);
    }

    void BfsShortestHops(const std::multimap<uint32, uint32>& jumps, uint32 start, uint32 maxDepth, std::map<uint32, int32>& outDist) {
        std::queue<std::pair<uint32, int32>> q;
        outDist.clear();
        outDist[start] = 0;
        q.push({ start, 0 });
        while (not q.empty()) {
            const uint32 u   = q.front().first;
            const int32 d    = q.front().second;
            q.pop();
            if (d >= static_cast<int32>(maxDepth))
                continue;
            const auto r = jumps.equal_range(u);
            for (auto it = r.first; it != r.second; ++it) {
                const uint32 v = it->second;
                if (outDist.find(v) != outDist.end())
                    continue;
                outDist[v] = d + 1;
                if (d + 1 < static_cast<int32>(maxDepth))
                    q.push({ v, d + 1 });
            }
        }
    }

    bool PickDropoffStation(uint32 agentStationID, uint32 systemID, uint32& outStationID) {
        std::vector<uint32> list;
        sDataMgr.GetStationList(systemID, list);
        if (list.empty())
            (void)StationDB::GetStationsInSolarSystem(systemID, list);
        if (list.empty())
            return false;
        for (uint8 n = 0; n < 20; ++n) {
            outStationID = list[MakeRandomInt(0, (uint32)(list.size() - 1))];
            if (outStationID != agentStationID or list.size() < 2)
                return true;
        }
        outStationID = list[0];
        return true;
    }

    /**
     * Courier / trade UIs call Pathfinder with destination solar system; destinationID 0 causes KeyError.
     * Used when range rules (e.g. distant low-sec) find no candidate or map jumps are sparse.
     */
    void EnsureCourierTradeStationDropoff(Agent* pAgent, bool station, uint8 misionType, MissionOffer& offer,
        const std::multimap<uint32, uint32>& systemJumps)
    {
        if (not station or offer.destinationID != 0 or pAgent == nullptr)
            return;
        if (misionType != Mission::Type::Courier and misionType != Mission::Type::Trade
            and misionType != Mission::Type::Mining)
            return;

        const uint32 agentSta  = pAgent->GetStationID();
        const uint32 originSys = pAgent->GetSystemID();
        uint32       st        = 0;

        std::map<uint32, int32> dist;
        BfsShortestHops(systemJumps, originSys, 96, dist);
        std::vector<uint32> sysCand;
        sysCand.reserve(dist.size());
        for (const auto& p : dist)
            sysCand.push_back(p.first);

        for (uint16 attempt = 0; attempt < 200 and offer.destinationID == 0; ++attempt) {
            if (sysCand.empty())
                break;
            const uint32 sys = sysCand[MakeRandomInt(0, (uint32)(sysCand.size() - 1))];
            if (PickDropoffStation(agentSta, sys, st))
                offer.destinationID = st;
        }

        if (offer.destinationID == 0) {
            std::vector<uint32> list;
            sDataMgr.GetStationList(originSys, list);
            if (list.empty())
                (void)StationDB::GetStationsInSolarSystem(originSys, list);
            for (uint32 sid : list) {
                if (sid != agentSta) {
                    offer.destinationID = sid;
                    break;
                }
            }
            if (offer.destinationID == 0 and not list.empty())
                offer.destinationID = list[0];
        }
        if (offer.destinationID == 0)
            offer.destinationID = agentSta;

        _log(AGENT__WARNING,
             "GetMissionDestination: courier/trade fallback dropoff station %u (agent sta %u, origin system %u, type %u).",
             offer.destinationID, agentSta, originSys, misionType);
    }

    void CollectLowSecByHopBand(const std::map<uint32, int32>& dist, uint32 originSys, int32 lo, int32 hi,
        std::vector<uint32>& out)
    {
        for (const auto& p : dist) {
            if (p.first == originSys)
                continue;
            if (p.second < lo or p.second > hi)
                continue;
            SystemData sd;
            if (not sDataMgr.GetSystemData(p.first, sd))
                continue;
            if (sd.securityRating < 0.5f)
                out.push_back(p.first);
        }
    }

    void CollectAnySysByHopBand(const std::map<uint32, int32>& dist, uint32 originSys, int32 lo, int32 hi,
        std::vector<uint32>& out)
    {
        for (const auto& p : dist) {
            if (p.first == originSys)
                continue;
            if (p.second < lo or p.second > hi)
                continue;
            out.push_back(p.first);
        }
    }
} // namespace


MapData::MapData()
: m_stationExtraInfo(nullptr),
m_pseudoSecurities(nullptr)
{
    m_regionJumps.clear();
    m_constJumps.clear();
    m_systemJumps.clear();
}

void MapData::Close()
{
    PySafeDecRef(m_stationExtraInfo);
    PySafeDecRef(m_pseudoSecurities);
}

int MapData::Initialize()
{
    Populate();
    sLog.Blue("          MapData", "Map Data Manager Initialized.");
    return 1;
}

void MapData::Clear()
{
    m_regionJumps.clear();
    m_constJumps.clear();
    m_systemJumps.clear();
}

void MapData::GetInfo()
{
    // print out list of bad jumps
    // m_badJumps
}

void MapData::Populate()
{
    m_pseudoSecurities = MapDB::GetPseudoSecurities();

    double start = GetTimeMSeconds();

    m_stationExtraInfo = new PyTuple(3);
    m_stationExtraInfo->items[0] = MapDB::GetStationExtraInfo();
    m_stationExtraInfo->items[1] = MapDB::GetStationOpServices();
    m_stationExtraInfo->items[2] = MapDB::GetStationServiceInfo();
    sLog.Cyan("          MapData", "StationExtraInfo loaded in %.3fms.",(GetTimeMSeconds() - start));

    start = GetTimeMSeconds();
    DBQueryResult* res = new DBQueryResult();
    MapDB::GetSystemJumps(*res);
    DBResultRow row;
    while (res->GetRow(row)) {
        //SELECT ctype, fromsol, tosol FROM mapConnections
        if (row.GetInt(0) == Map::Jumptype::Region) {
            m_regionJumps.emplace(row.GetInt(1), row.GetInt(2));
        } else if (row.GetInt(0) == Map::Jumptype::Constellation) {
            m_constJumps.emplace(row.GetInt(1), row.GetInt(2));
        } else {
            m_systemJumps.emplace(row.GetInt(1), row.GetInt(2));
        }
    }

    sLog.Cyan("          MapData", "%lu Region jumps, %lu Constellation jumps and %lu System jumps loaded in %.3fms.", //
              m_regionJumps.size(), m_constJumps.size(), m_systemJumps.size(), (GetTimeMSeconds() - start));

    // cleanup
    SafeDelete(res);
}



void MapData::GetMissionDestination(Agent* pAgent, uint8 misionType, MissionOffer& offer)
{
    using namespace Mission::Type;
    using namespace Agents::Range;

    uint8 destRange = offer.range;
    bool station = true, ship = false;    // will have to tweak this later for particular mission events

    // determine distance based on preset range from db or in some cases, mission type and agent level
    switch(misionType) {
        case Tutorial: {
            // always same system?
            destRange = SameSystem;
        } break;
        case Data:
        case Trade:
        case Courier:
        case Research: {
            //destRange += m_data.level *2;
        } break;
        case EpicArc:
        case Anomic:
        case Burner:
        case Cosmos: {
            station = false;
            destRange += pAgent->GetLevel();
        } break;
        case Mining: {
            station = true;
        } break;
        case Encounter:
        case Storyline: {
            station = true;
        } break;
    }

    switch(destRange) {
        case 0:
        case SameSystem: //1
        case SameOrNeighboringSystemSameConstellation:    //2
        case NeighboringSystemSameConstellation:  //4
        case SameConstellation:  {  //6
            uint32 systemID = pAgent->GetSystemID();
            if (station)
                if (sDataMgr.GetStationCount(systemID) < 2)
                    ++destRange;

            if ((destRange > 1) or (IsEven(MakeRandomInt(0, 100)))) {
                // neighboring system
                bool run = true;
                uint8 count = 0;
                std::vector<uint32> sysList;
                auto itr = m_systemJumps.equal_range(systemID);
                for (auto it = itr.first; it != itr.second; ++it)
                    sysList.push_back(it->second);
                if (sysList.empty()) {
                    // No edges in map data (e.g. isolated system): keep systemID and let the in-system station
                    // selection below set destinationID. Early return with destinationID unset breaks the
                    // client (dropoffLocation must include locationID for courier objectives).
                    _log(AGENT__WARNING, "GetMissionDestination() - no stargate jumps from system %u; using in-system dropoff", systemID);
                } else while (run) {
                    run = false;
                    uint32 randomIndex = MakeRandomInt(0, (sysList.size() -1));
                    systemID = sysList.at(randomIndex);
                    if (station and (sDataMgr.GetStationCount(systemID) < 1)) {
                        run = true;
                        ++count;
                    }
                    if (run and (count > sysList.size())) {
                        // problem....no station found within one jump
                        offer.destinationID = 0;
                        _log(AGENT__ERROR, "Agent::GetMissionDestination() - no station found within 1 jump." );
                        run = false;
                    }
                    sysList.erase(sysList.begin() + randomIndex); // If we have searched a system already then do not try it again
                }
            }
            if (station) {
                std::vector<uint32> list;
                sDataMgr.GetStationList(systemID, list);
                if (list.empty()) {
                    offer.destinationID = 0;
                } else if (list.size() < 2) {
                    offer.destinationID = list[0];
                } else {
                    bool run = true;
                    while (run) {
                        offer.destinationID = list.at(MakeRandomInt(0, (list.size() -1)));
                        if (offer.destinationID != pAgent->GetStationID())
                            run = false;
                    }
                }
            } else if (ship) {
                ;  // code here for agent in ship
            }
        } break;

        case WithinTwoJumpsOfAgent: {
            // EVEmu Juro local haul: same system or 1-2 stargate hops, station dropoff
            const uint32 agentSta  = pAgent->GetStationID();
            const uint32 originSys = pAgent->GetSystemID();
            std::map<uint32, int32> dist;
            BfsShortestHops(m_systemJumps, originSys, 2, dist);
            std::vector<uint32> cand;
            cand.reserve(dist.size());
            for (const auto& p : dist) {
                if (p.second <= 2)
                    cand.push_back(p.first);
            }
            for (uint8 attempt = 0; attempt < 50 && offer.destinationID == 0; ++attempt) {
                if (cand.empty())
                    break;
                const uint32 sys = cand[MakeRandomInt(0, (uint32)(cand.size() - 1))];
                uint32         st  = 0;
                if (PickDropoffStation(agentSta, sys, st))
                    offer.destinationID = st;
            }
        } break;

        case WithinThirteenJumpsOfAgent: {
            // 1–13 jumps: not same-system station hops (enum 13 is only 0–2 jumps and includes origin).
            const uint32 agentSta  = pAgent->GetStationID();
            const uint32 originSys = pAgent->GetSystemID();
            std::map<uint32, int32> dist;
            BfsShortestHops(m_systemJumps, originSys, 14, dist);
            std::vector<uint32> cand;
            for (const auto& p : dist) {
                if (p.first == originSys)
                    continue;
                if (p.second < 1 or p.second > 13)
                    continue;
                cand.push_back(p.first);
            }
            if (cand.empty())
                CollectAnySysByHopBand(dist, originSys, 1, 13, cand);
            if (cand.empty())
                CollectAnySysByHopBand(dist, originSys, 1, 25, cand);
            for (uint16 attempt = 0; attempt < 150 and offer.destinationID == 0; ++attempt) {
                if (cand.empty())
                    break;
                const uint32 sys = cand[MakeRandomInt(0, (uint32)(cand.size() - 1))];
                uint32         st  = 0;
                if (PickDropoffStation(agentSta, sys, st))
                    offer.destinationID = st;
            }
            if (offer.destinationID == 0)
                _log(AGENT__WARNING,
                     "GetMissionDestination: WithinThirteenJumps (range=15) found no dropoff (origin system %u, agent station %u).",
                     originSys, agentSta);
        } break;

        case DistantLowSecEightToFifteenJumps: {
            // 8-15 jumps, low-sec (security < 0.5) or null-sec. Deep BFS + relaxed bands so we never fall back
            // to RefillOfferSystemIds pinning the agent's station (same-station "long haul" bug).
            const uint32 agentSta  = pAgent->GetStationID();
            const uint32 originSys = pAgent->GetSystemID();
            std::map<uint32, int32> dist;
            BfsShortestHops(m_systemJumps, originSys, 48, dist);
            std::vector<uint32> cand;
            CollectLowSecByHopBand(dist, originSys, 8, 15, cand);
            if (cand.empty())
                CollectLowSecByHopBand(dist, originSys, 5, 20, cand);
            if (cand.empty())
                CollectLowSecByHopBand(dist, originSys, 3, 35, cand);
            if (cand.empty())
                CollectAnySysByHopBand(dist, originSys, 8, 18, cand);
            if (cand.empty())
                CollectAnySysByHopBand(dist, originSys, 5, 40, cand);
            for (uint8 attempt = 0; attempt < 100 && offer.destinationID == 0; ++attempt) {
                if (cand.empty())
                    break;
                const uint32 sys = cand[MakeRandomInt(0, (uint32)(cand.size() - 1))];
                uint32         st  = 0;
                if (PickDropoffStation(agentSta, sys, st))
                    offer.destinationID = st;
            }
            if (offer.destinationID == 0)
                _log(AGENT__ERROR,
                     "GetMissionDestination: DistantLowSec (range=14) found no dropoff (origin system %u, agent station %u, BFS systems %lu).",
                     originSys, agentSta, (unsigned long)dist.size());
        } break;

        //may have to create data objects based on constellation to do ranges in neighboring constellation
        // could use data from mapSolarSystemJumps - fromRegionID, fromConstellationID, fromSolarSystemID, toSolarSystemID, toConstellationID, toRegionID

        /** @todo  make function to find route from origin to constellation/region jump point.  */
        case SameOrNeighboringSystem:  //3
        case NeighboringSystem: {  //5
            uint32 systemID = pAgent->GetSystemID();
            if (IsEven(MakeRandomInt(0, 100))) {
                // same constellation
            } else {
                // neighboring constellation
                systemID = 0;
            }
            if (ship) {
                ;  // code here for agent in ship
            }
        } break;
        case SameOrNeighboringConstellationSameRegion:   //7
        case NeighboringConstellationSameRegion: {  //9
            if (station)
                sDataMgr.GetStationConstellation(pAgent->GetStationID());
        } break;
        case SameOrNeighboringConstellation:  //8
        case NeighboringConstellation: {    //10
            if (station)
                sDataMgr.GetStationRegion(pAgent->GetStationID());
        } break;
        // not sure how to do these two yet....
        case NearestEnemyCombatZone: {  //11
        } break;
        case NearestCareerHub: {    //12
        } break;
    }

    EnsureCourierTradeStationDropoff(pAgent, station, misionType, offer, m_systemJumps);

    // Some branches leave a solar system id in destinationID; couriers need a real station (client UI
    // expects dropoff locationID in the 60M station range).
    if ((misionType == Courier or misionType == Trade or misionType == Mining) and offer.destinationID != 0
        and IsSolarSystemID(offer.destinationID)
        and (not (sDataMgr.IsStation(offer.destinationID) or IsStationID(offer.destinationID))))
    {
        std::vector<uint32> stList;
        if (not sDataMgr.GetStationList(offer.destinationID, stList) or stList.empty())
            (void)StationDB::GetStationsInSolarSystem(offer.destinationID, stList);
        if (not stList.empty()) {
            uint32 pick = stList[0];
            for (uint32 sid : stList) {
                if (sid != pAgent->GetStationID()) {
                    pick = sid;
                    break;
                }
            }
            offer.destinationID = pick;
        }
    }

    if (offer.destinationID != 0 and
        (sDataMgr.IsStation(offer.destinationID) or IsStationID(offer.destinationID))) {
        StationData data = StationData();
        stDataMgr.GetStationData(offer.destinationID, data);
        offer.destinationOwnerID    = data.corporationID;
        offer.destinationSystemID   = data.systemID;
        offer.destinationTypeID     = data.typeID;
    } else if (ship) {
        offer.destinationSystemID   = offer.destinationID;
        offer.destinationTypeID     = sDataMgr.GetStaticType(offer.destinationID);
        offer.dungeonLocationID     = offer.destinationID;
        offer.dungeonSolarSystemID  = offer.destinationID;
    } else {
        offer.destinationSystemID   = offer.destinationID;
        offer.destinationTypeID     = sDataMgr.GetStaticType(offer.destinationID);
        offer.dungeonLocationID     = offer.destinationID;
        offer.dungeonSolarSystemID  = offer.destinationID;
    }
}

void MapData::GetPlanets(uint32 systemID) {
    uint8 total = 0;
    std::vector<DBGPointEntity> planetIDs;
    planetIDs.clear();
    MapDB::GetPlanets(systemID, planetIDs, total);
}

void MapData::GetMoons(uint32 systemID) {
    uint8 total = 0;
    std::vector<DBGPointEntity> moonIDs;
    moonIDs.clear();
    MapDB::GetMoons(systemID, moonIDs, total);
}

const GPoint MapData::GetRandPointOnPlanet(uint32 systemID) {
    uint8 total = 0;
    std::vector<DBGPointEntity> planetIDs;
    planetIDs.clear();
    MapDB::GetPlanets(systemID, planetIDs, total);

    if (planetIDs.empty())
        return NULL_ORIGIN;

    uint16 i = MakeRandomInt(1, total);
    return (planetIDs[i].position + planetIDs[i].radius + 50000);
}

const GPoint MapData::GetRandPointOnMoon(uint32 systemID) {
    uint8 total = 0;
    std::vector<DBGPointEntity> moonIDs;
    moonIDs.clear();
    MapDB::GetMoons(systemID, moonIDs, total);

    if (moonIDs.empty())
        return NULL_ORIGIN;

    uint16 i = MakeRandomInt(1, total);
    return (moonIDs[i].position + moonIDs[i].radius + 10000);
}

uint32 MapData::GetRandPlanet(uint32 systemID) {
    uint8 total = 0;
    std::vector<DBGPointEntity> planetIDs;
    planetIDs.clear();
    MapDB::GetPlanets(systemID, planetIDs, total);

    if (planetIDs.empty())
        return 0;

    uint16 i = MakeRandomInt(1, total);
    return planetIDs[i].itemID;
}

const GPoint MapData::Get2RandPlanets(uint32 systemID) {
    uint8 total = 0;
    std::vector<DBGPointEntity> planetIDs;
    planetIDs.clear();
    MapDB::GetPlanets(systemID, planetIDs, total);
    /** @todo finish this */
    return NULL_ORIGIN;
}

const GPoint MapData::Get3RandPlanets(uint32 systemID) {
    uint8 total = 0;
    std::vector<DBGPointEntity> planetIDs;
    planetIDs.clear();
    MapDB::GetPlanets(systemID, planetIDs, total);
    /** @todo finish this */

    return NULL_ORIGIN;
}

uint32 MapData::GetRandMoon(uint32 systemID) {
    uint8 total = 0;
    std::vector<DBGPointEntity> moonIDs;
    moonIDs.clear();
    MapDB::GetMoons(systemID, moonIDs, total);

    if (moonIDs.empty())
        return 0;

    uint16 i = MakeRandomInt(1, total);
    return moonIDs[i].itemID;
}

const GPoint MapData::GetRandPointInSystem(uint32 systemID, int64 distance) {
    // get system max diameter, verify distance is within system.

    return NULL_ORIGIN;
}

const GPoint MapData::GetAnomalyPoint(SystemManager* pSys)
{
    return AnomalyAnchorForSystem(pSys, pSys->GetID());
}

const GPoint MapData::GetAnomalyPoint(uint32 systemID)
{
    return AnomalyAnchorForSystem(nullptr, systemID);
}