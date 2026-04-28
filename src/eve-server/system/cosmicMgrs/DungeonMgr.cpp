 /**
  * @name DungeonMgr.cpp
  *     Dungeon management system for EVEmu
  *
  * @Author:        James
  * @date:          13 December 2022
  */
 
#include "eve-server.h"

#include "EVEServerConfig.h"

#include <algorithm>
#include <unordered_set>

#include "StaticDataMgr.h"
#include "dungeon/DungeonDB.h"
#include "system/SystemBubble.h"
#include "system/cosmicMgrs/SpawnMgr.h"
#include "system/cosmicMgrs/AnomalyMgr.h"
#include "system/cosmicMgrs/BeltMgr.h"
#include "system/cosmicMgrs/DungeonMgr.h"

#include "../../../eve-common/EVE_Corp.h"
#include "../../../eve-common/EVE_Dungeon.h"
#include "system/BubbleManager.h"

namespace {
    enum HullKind : uint8 {
        HK_Frigate = 0,
        HK_Destroyer,
        HK_Cruiser,
        HK_Battlecruiser,
        HK_Battleship
    };

    struct AnomPirateGroups {
        uint16 frigate;
        uint16 cruiser;
        uint16 battlecruiser;
        uint16 battleship;
        uint16 cmdFrigate;
        uint16 cmdCruiser;
        uint16 cmdBattlecruiser;
        uint16 cmdBattleship;
        /** Asteroid * Destroyer (category 11); 0 = fall back to frigate group. */
        uint16 destroyer;
    };

    bool GetAnomPirateGroups(uint32 factionID, AnomPirateGroups& g)
    {
        // Asteroid * pirate Entity groups (category 11) — see https://everef.net/categories/11
        switch (factionID) {
            case factionAngel:
                g = {550, 551, 576, 552, 789, 790, 793, 848, 553};
                return true;
            case factionGuristas:
                g = {562, 561, 580, 560, 800, 798, 797, 850, 559};
                return true;
            case factionBloodRaider:
                g = {557, 555, 578, 556, 792, 791, 795, 849, 558};
                return true;
            case factionSerpentis:
                g = {572, 571, 584, 570, 814, 812, 811, 852, 573};
                return true;
            case factionSanshas:
                g = {567, 566, 582, 565, 810, 808, 807, 851, 568};
                return true;
            case factionRogueDrones:
                g = {759, 757, 755, 756, 847, 845, 843, 844, 758};
                return true;
            case factionMordusLegion:
                // Only commander asteroid groups exist for Mordus (1285–1287); reuse per hull class.
                g = {1285, 1286, 1286, 1287, 1285, 1286, 1287, 1287, 0};
                return true;
            default:
                g = {550, 551, 576, 552, 789, 790, 793, 848, 553};
                return false;
        }
    }

    struct SpawnLineSpec {
        HullKind hull;
        bool elite;
        uint8 minC;
        uint8 maxC;
    };

    struct WaveSpec {
        uint8 lineCount;
        SpawnLineSpec lines[6];
    };

    uint16 ResolveSpawnGroup(const AnomPirateGroups& g, HullKind hull, bool elite)
    {
        if (elite) {
            switch (hull) {
                case HK_Frigate:       return g.cmdFrigate;
                case HK_Destroyer:     return g.cmdCruiser; /* commander destroyer proxy */
                case HK_Cruiser:       return g.cmdCruiser;
                case HK_Battlecruiser: return g.cmdBattlecruiser;
                case HK_Battleship:    return g.cmdBattleship;
            }
        }
        switch (hull) {
            case HK_Frigate:       return g.frigate;
            case HK_Destroyer:     return g.destroyer ? g.destroyer : g.frigate;
            case HK_Cruiser:       return g.cruiser;
            case HK_Battlecruiser: return g.battlecruiser;
            case HK_Battleship:    return g.battleship;
        }
        return 0;
    }

    uint8 ClassifyAnomalySiteKind(const std::string& dungeonName)
    {
        std::string n = dungeonName;
        if (n.size() >= 8u && n.compare(0u, 8u, "Teeming ") == 0)
            n.erase(0u, 8u);

        static const struct { const char* prefix; size_t len; } factions[] = {
            { "Guristas ", 9 },
            { "Serpentis ", 11 },
            { "Sansha ", 7 },
            { "Angel ", 6 },
            { "Blood ", 6 },
        };
        for (const auto& f : factions) {
            if (n.size() >= f.len && n.compare(0u, f.len, f.prefix) == 0) {
                n.erase(0u, f.len);
                break;
            }
        }

        /* Rogue Drone names ↔ tier (EVE combat anomaly chart): Collection=Burrow, Assembly=Refuge, … */
        if (n.size() >= 6u && n.compare(0u, 6u, "Drone ") == 0) {
            if (n == "Drone Cluster")
                return Dungeon::AnomalySiteKind::Hideaway;
            if (n == "Drone Collection")
                return Dungeon::AnomalySiteKind::Burrow;
            if (n == "Drone Assembly")
                return Dungeon::AnomalySiteKind::Refuge;
            if (n == "Drone Gathering")
                return Dungeon::AnomalySiteKind::Den;
            if (n == "Drone Surveillance")
                return Dungeon::AnomalySiteKind::Yard;
            if (n == "Drone Menagerie")
                return Dungeon::AnomalySiteKind::RallyPoint;
            if (n == "Drone Herd")
                return Dungeon::AnomalySiteKind::Port;
            if (n == "Drone Squad")
                return Dungeon::AnomalySiteKind::Hub;
            if (n == "Drone Patrol")
                return Dungeon::AnomalySiteKind::Haven;
            if (n == "Drone Horde")
                return Dungeon::AnomalySiteKind::Sanctum;
        }

        for (;;) {
            if (n.size() >= 7u && n.compare(0u, 7u, "Hidden ") == 0) {
                n.erase(0u, 7u);
                continue;
            }
            if (n.size() >= 9u && n.compare(0u, 9u, "Forsaken ") == 0) {
                n.erase(0u, 9u);
                continue;
            }
            if (n.size() >= 8u && n.compare(0u, 8u, "Forlorn ") == 0) {
                n.erase(0u, 8u);
                continue;
            }
            break;
        }

        if (n == "Rally Point")
            return Dungeon::AnomalySiteKind::RallyPoint;
        if (n == "Hideaway")
            return Dungeon::AnomalySiteKind::Hideaway;
        if (n == "Refuge")
            return Dungeon::AnomalySiteKind::Refuge;
        if (n == "Burrow")
            return Dungeon::AnomalySiteKind::Burrow;
        if (n == "Den")
            return Dungeon::AnomalySiteKind::Den;
        if (n == "Yard")
            return Dungeon::AnomalySiteKind::Yard;
        if (n == "Port")
            return Dungeon::AnomalySiteKind::Port;
        if (n == "Hub")
            return Dungeon::AnomalySiteKind::Hub;
        if (n == "Haven")
            return Dungeon::AnomalySiteKind::Haven;
        if (n == "Sanctum")
            return Dungeon::AnomalySiteKind::Sanctum;

        return Dungeon::AnomalySiteKind::Unknown;
    }

    /** Hidden / Forsaken / Forlorn Den spawn in low+null only; base Den in high+low. */
    bool IsDenVariantDungeonName(const std::string& dungeonName)
    {
        return dungeonName.find(" Hidden Den") != std::string::npos
            || dungeonName.find(" Forsaken Den") != std::string::npos
            || dungeonName.find(" Forlorn Den") != std::string::npos;
    }

    /**
     * mapSecurityStatus = mapSolarSystems.security (client UI): ≥0.5 high, (0,0.5) low, ≤0 null.
     * Reference: standard combat anomaly spawn by sec band.
     */
    bool CombatAnomalyAllowedAtMapSec(uint8 siteKind, float mapSec, bool denIsVariant)
    {
        const bool highSec = (mapSec >= 0.5f);
        const bool lowSecOnly = (mapSec > 0.0f && mapSec < 0.5f);
        const bool lowOrNull = (mapSec < 0.5f);
        const bool nullSec = (mapSec <= 0.0f);
        const bool highOrLow = (mapSec > 0.0f);

        switch (siteKind) {
            case Dungeon::AnomalySiteKind::Hideaway:
            case Dungeon::AnomalySiteKind::Refuge:
                return highOrLow;
            case Dungeon::AnomalySiteKind::Burrow:
                return highSec;
            case Dungeon::AnomalySiteKind::Den:
                if (denIsVariant)
                    return lowOrNull;
                return highOrLow;
            case Dungeon::AnomalySiteKind::Yard:
                return lowSecOnly;
            case Dungeon::AnomalySiteKind::RallyPoint:
            case Dungeon::AnomalySiteKind::Port:
            case Dungeon::AnomalySiteKind::Hub:
                return lowOrNull;
            case Dungeon::AnomalySiteKind::Haven:
            case Dungeon::AnomalySiteKind::Sanctum:
                return nullSec;
            default:
                return true;
        }
    }

    /* UniWiki Angel * baseline — composition only; faction uses GetAnomPirateGroups. */
    static const WaveSpec kWavesHideaway[] = {
        { 1, { { HK_Frigate, false, 1, 2 } } },
        { 1, { { HK_Frigate, false, 1, 2 } } },
        { 1, { { HK_Frigate, false, 1, 3 } } },
        { 1, { { HK_Frigate, false, 1, 3 } } },
        { 1, { { HK_Frigate, false, 1, 3 } } },
    };

    static const WaveSpec kWavesRefuge[] = {
        { 1, { { HK_Frigate, false, 2, 3 } } },
        { 1, { { HK_Frigate, false, 2, 3 } } },
        { 2, { { HK_Frigate, false, 1, 3 }, { HK_Destroyer, false, 0, 3 } } },
        { 2, { { HK_Frigate, false, 0, 3 }, { HK_Destroyer, false, 1, 3 } } },
        { 2, { { HK_Frigate, false, 0, 3 }, { HK_Destroyer, false, 1, 2 } } },
    };

    static const WaveSpec kWavesBurrow[] = {
        { 1, { { HK_Frigate, false, 6, 6 } } },
        { 1, { { HK_Frigate, false, 3, 4 } } },
        { 1, { { HK_Frigate, false, 2, 2 } } },
        { 1, { { HK_Frigate, false, 3, 3 } } },
        { 1, { { HK_Frigate, false, 1, 1 } } },
    };

    static const WaveSpec kWavesDen[] = {
        { 2, { { HK_Frigate, false, 6, 6 }, { HK_Destroyer, false, 2, 2 } } },
        { 2, { { HK_Destroyer, false, 2, 2 }, { HK_Cruiser, false, 2, 2 } } },
        { 2, { { HK_Frigate, false, 2, 2 }, { HK_Cruiser, false, 2, 2 } } },
        { 2, { { HK_Destroyer, false, 2, 2 }, { HK_Cruiser, false, 3, 3 } } },
        { 2, { { HK_Destroyer, false, 5, 5 }, { HK_Cruiser, false, 5, 5 } } },
    };

    static const WaveSpec kWavesYard[] = {
        { 1, { { HK_Cruiser, false, 3, 4 } } },
        { 1, { { HK_Cruiser, false, 3, 4 } } },
        { 2, { { HK_Frigate, false, 3, 3 }, { HK_Cruiser, false, 2, 3 } } },
        { 2, { { HK_Destroyer, false, 3, 4 }, { HK_Cruiser, false, 3, 4 } } },
        { 2, { { HK_Cruiser, false, 1, 1 }, { HK_Battleship, false, 1, 1 } } },
    };

    static const WaveSpec kWavesRally[] = {
        { 2, { { HK_Destroyer, false, 3, 4 }, { HK_Cruiser, false, 3, 4 } } },
        { 2, { { HK_Frigate, true, 2, 3 }, { HK_Cruiser, false, 2, 3 } } },
        { 2, { { HK_Cruiser, false, 3, 4 }, { HK_Battlecruiser, false, 3, 4 } } },
        { 2, { { HK_Battlecruiser, false, 1, 2 }, { HK_Battleship, false, 1, 2 } } },
        { 2, { { HK_Battlecruiser, false, 1, 2 }, { HK_Battleship, false, 1, 2 } } },
    };

    static const WaveSpec kWavesPort[] = {
        { 2, { { HK_Frigate, true, 3, 4 }, { HK_Battlecruiser, false, 3, 4 } } },
        { 2, { { HK_Frigate, true, 2, 3 }, { HK_Battlecruiser, false, 2, 3 } } },
        { 3, { { HK_Destroyer, false, 3, 4 }, { HK_Battlecruiser, false, 6, 7 }, { HK_Battleship, false, 0, 3 } } },
        { 2, { { HK_Battlecruiser, false, 3, 4 }, { HK_Battleship, false, 2, 4 } } },
    };

    static const WaveSpec kWavesHub[] = {
        { 2, { { HK_Frigate, true, 2, 3 }, { HK_Battleship, false, 2, 3 } } },
        { 2, { { HK_Battlecruiser, false, 4, 4 }, { HK_Battleship, false, 3, 4 } } },
        { 3, { { HK_Destroyer, false, 3, 4 }, { HK_Battlecruiser, false, 2, 3 }, { HK_Battleship, false, 4, 4 } } },
        { 2, { { HK_Cruiser, true, 3, 3 }, { HK_Battleship, false, 4, 4 } } },
    };

    static const WaveSpec kWavesHaven[] = {
        { 3, { { HK_Frigate, true, 3, 3 }, { HK_Battlecruiser, false, 2, 2 }, { HK_Battleship, false, 2, 2 } } },
        { 3, { { HK_Cruiser, true, 2, 2 }, { HK_Battlecruiser, false, 3, 3 }, { HK_Battleship, false, 2, 2 } } },
        { 3, { { HK_Frigate, false, 3, 3 }, { HK_Battlecruiser, false, 4, 4 }, { HK_Battleship, false, 3, 3 } } },
        { 3, { { HK_Frigate, true, 2, 2 }, { HK_Battlecruiser, false, 4, 4 }, { HK_Battleship, false, 4, 4 } } },
        { 2, { { HK_Battlecruiser, false, 6, 6 }, { HK_Battleship, false, 4, 4 } } },
        { 2, { { HK_Frigate, true, 3, 3 }, { HK_Battleship, false, 6, 6 } } },
        { 2, { { HK_Battlecruiser, false, 4, 4 }, { HK_Battleship, false, 6, 6 } } },
        { 1, { { HK_Battleship, false, 8, 8 } } },
    };

    static const WaveSpec kWavesSanctum[] = {
        { 3, { { HK_Frigate, true, 3, 3 }, { HK_Battlecruiser, false, 2, 2 }, { HK_Battleship, false, 2, 2 } } },
        { 3, { { HK_Cruiser, true, 2, 2 }, { HK_Battlecruiser, false, 3, 3 }, { HK_Battleship, false, 2, 2 } } },
        { 3, { { HK_Frigate, false, 3, 3 }, { HK_Battlecruiser, false, 4, 4 }, { HK_Battleship, false, 3, 3 } } },
        { 3, { { HK_Frigate, true, 2, 2 }, { HK_Battlecruiser, false, 4, 4 }, { HK_Battleship, false, 4, 4 } } },
        { 2, { { HK_Battlecruiser, false, 6, 6 }, { HK_Battleship, false, 4, 4 } } },
        { 2, { { HK_Frigate, true, 3, 3 }, { HK_Battleship, false, 6, 6 } } },
        { 2, { { HK_Battlecruiser, false, 4, 4 }, { HK_Battleship, false, 6, 6 } } },
        { 1, { { HK_Battleship, false, 8, 8 } } },
        { 2, { { HK_Battleship, true, 3, 4 }, { HK_Battlecruiser, true, 2, 3 } } },
    };

    bool GetWaveTable(uint8 kind, const WaveSpec*& outTable, uint8& outCount)
    {
        switch (kind) {
            case Dungeon::AnomalySiteKind::Hideaway:
                outTable = kWavesHideaway;
                outCount = sizeof(kWavesHideaway) / sizeof(kWavesHideaway[0]);
                return true;
            case Dungeon::AnomalySiteKind::Refuge:
                outTable = kWavesRefuge;
                outCount = sizeof(kWavesRefuge) / sizeof(kWavesRefuge[0]);
                return true;
            case Dungeon::AnomalySiteKind::Burrow:
                outTable = kWavesBurrow;
                outCount = sizeof(kWavesBurrow) / sizeof(kWavesBurrow[0]);
                return true;
            case Dungeon::AnomalySiteKind::Den:
                outTable = kWavesDen;
                outCount = sizeof(kWavesDen) / sizeof(kWavesDen[0]);
                return true;
            case Dungeon::AnomalySiteKind::Yard:
                outTable = kWavesYard;
                outCount = sizeof(kWavesYard) / sizeof(kWavesYard[0]);
                return true;
            case Dungeon::AnomalySiteKind::RallyPoint:
                outTable = kWavesRally;
                outCount = sizeof(kWavesRally) / sizeof(kWavesRally[0]);
                return true;
            case Dungeon::AnomalySiteKind::Port:
                outTable = kWavesPort;
                outCount = sizeof(kWavesPort) / sizeof(kWavesPort[0]);
                return true;
            case Dungeon::AnomalySiteKind::Hub:
                outTable = kWavesHub;
                outCount = sizeof(kWavesHub) / sizeof(kWavesHub[0]);
                return true;
            case Dungeon::AnomalySiteKind::Haven:
                outTable = kWavesHaven;
                outCount = sizeof(kWavesHaven) / sizeof(kWavesHaven[0]);
                return true;
            case Dungeon::AnomalySiteKind::Sanctum:
                outTable = kWavesSanctum;
                outCount = sizeof(kWavesSanctum) / sizeof(kWavesSanctum[0]);
                return true;
            default:
                outTable = nullptr;
                outCount = 0;
                return false;
        }
    }

    void SpawnWaveFromSpec(DungeonMgr* mgr, Dungeon::LiveDungeon& ld, const AnomPirateGroups& pg, const WaveSpec& w)
    {
        if (mgr == nullptr)
            return;
        for (uint8 i = 0; i < w.lineCount; ++i) {
            const SpawnLineSpec& L = w.lines[i];
            const uint16 gid = ResolveSpawnGroup(pg, L.hull, L.elite);
            if (!gid)
                continue;
            uint8 c = L.minC;
            if (L.maxC > L.minC)
                c = static_cast<uint8>(L.minC + (rand() % (L.maxC - L.minC + 1u)));
            if (c == 0)
                continue;
            mgr->SpawnAnomalyTypeNearAnchor(ld, gid, c, L.elite);
        }
    }

    void SpawnAnomalyWaveRatsLegacy(DungeonMgr* mgr, Dungeon::LiveDungeon& ld, uint8 waveIndex, const AnomPirateGroups& pg)
    {
        if (mgr == nullptr)
            return;
        if (waveIndex == 1) {
            mgr->SpawnAnomalyTypeNearAnchor(ld, pg.frigate, static_cast<uint8>(3 + (rand() % 2)), false);
        } else if (waveIndex == 2) {
            mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cruiser, 2, false);
            if ((rand() % 3) != 0)
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.frigate, 1, false);
        } else if (waveIndex >= 3) {
            if ((rand() % 2) != 0) {
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.battlecruiser, 1, false);
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cruiser, 1, false);
            } else {
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.battleship, 1, false);
            }
        }
    }

    void SpawnAnomalyCommanderRatsTiered(DungeonMgr* mgr, Dungeon::LiveDungeon& ld, const AnomPirateGroups& pg)
    {
        if (mgr == nullptr)
            return;
        const uint8 k = ld.anomalySiteKind;
        _log(COSMIC_MGR__MESSAGE, "DungeonMgr::SpawnAnomalyCommanderRats - commander wave for anomaly %u siteKind=%u", ld.anomalyID, (unsigned)k);

        if (k == Dungeon::AnomalySiteKind::Hideaway || k == Dungeon::AnomalySiteKind::Burrow) {
            if ((rand() % 2) == 0)
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdFrigate, 1, true);
            return;
        }
        if (k == Dungeon::AnomalySiteKind::Refuge) {
            if ((rand() % 2) == 0)
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdCruiser, 1, true);
            return;
        }
        if (k == Dungeon::AnomalySiteKind::Den || k == Dungeon::AnomalySiteKind::Yard) {
            if ((rand() % 2) == 0)
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdCruiser, 1, true);
            return;
        }
        if (k == Dungeon::AnomalySiteKind::RallyPoint || k == Dungeon::AnomalySiteKind::Port) {
            if ((rand() % 2) == 0) {
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdBattlecruiser, 1, true);
            } else {
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdCruiser, 1, true);
            }
            return;
        }
        if (k == Dungeon::AnomalySiteKind::Hub || k == Dungeon::AnomalySiteKind::Haven || k == Dungeon::AnomalySiteKind::Sanctum) {
            if ((rand() % 2) != 0) {
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdBattleship, 1, true);
                if ((rand() % 3) == 0)
                    mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdFrigate, 1, true);
            } else {
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdCruiser, static_cast<uint8>(1 + (rand() % 2)), true);
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdBattlecruiser, 1, true);
            }
            return;
        }
        /* Drone ladder / unknown scripted kinds: BS-heavy commander mix */
        if ((rand() % 2) != 0) {
            mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdBattleship, 1, true);
            if ((rand() % 3) == 0)
                mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdFrigate, 1, true);
        } else {
            mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdCruiser, static_cast<uint8>(1 + (rand() % 2)), true);
            mgr->SpawnAnomalyTypeNearAnchor(ld, pg.cmdFrigate, 1, true);
        }
    }

    void RunAnomalyProfileWave(DungeonMgr* mgr, Dungeon::LiveDungeon& ld, uint8 waveIndex)
    {
        if (mgr == nullptr)
            return;
        AnomPirateGroups pg{};
        GetAnomPirateGroups(ld.templateFactionID, pg);
        _log(COSMIC_MGR__MESSAGE, "DungeonMgr::SpawnAnomalyWaveRats - anomaly %u siteKind=%u wave %u/%u",
            ld.anomalyID, (unsigned)ld.anomalySiteKind, (unsigned)waveIndex, (unsigned)ld.anomalyRegularWaveCount);

        if (ld.anomalySiteKind == Dungeon::AnomalySiteKind::Unknown) {
            SpawnAnomalyWaveRatsLegacy(mgr, ld, waveIndex, pg);
            return;
        }
        const WaveSpec* table = nullptr;
        uint8 nWaves = 0;
        if (!GetWaveTable(ld.anomalySiteKind, table, nWaves) || table == nullptr || nWaves == 0 || waveIndex < 1)
            return;
        if (waveIndex > nWaves)
            return;
        SpawnWaveFromSpec(mgr, ld, pg, table[waveIndex - 1]);
    }
}

/*
Dungeon flow:
1. load dungeons from db on init
2. when signature/anomaly is created, spawn first room of dungeon at ship position (how to handle multi-room dungeons?)

Multi-room dungeons have each room with acceleration gate pointing to next room (accel gate is not implemented????)

*/


DungeonDataMgr::DungeonDataMgr()
    : m_dungeonID(0)
{
    m_dungeons.clear();
}

int DungeonDataMgr::Initialize()
{
    // Loads all dungeons from database upon server initialisation
    Populate();
    sLog.Blue("       DunDataMgr", "Dungeon Data Manager Initialized.");
    return 1;
}

void DungeonDataMgr::UpdateDungeon(uint32 dungeonID)
{
    _log(DUNG__INFO, "UpdateDungeon() - Updating dungeon %u's object in DataMgr...", dungeonID);
    // Get dungeon from DB by dungeonID
    DBQueryResult *res = new DBQueryResult();
    DBResultRow row;
    DungeonDB::GetAllDungeonDataByDungeonID(*res, dungeonID);

    // Multi-index view by dungeonID
    auto &byDungeonID = m_dungeons.get<Dungeon::DungeonsByID>();

    // Update a dungeon's in-memory object
    auto it = byDungeonID.find(dungeonID);
    if (it != byDungeonID.end()) {
        // Dungeon already exists, update in-memory object
        byDungeonID.erase(dungeonID);

        if (res->GetRowCount() > 0) {
            while (res->GetRow(row))
            {
                CreateDungeon(row);
                FillObject(row);
            }
        } else {
            _log(DUNG__ERROR, "UpdateDungeon() - Failed to find dungeon %u in database. This should never happen.", dungeonID);
        }

    } else {
        _log(DUNG__ERROR, "UpdateDungeon() - Failed to find dungeon %u's object in DataMgr. This should never happen.", dungeonID);
    }
    SafeDelete(res);
}

void DungeonDataMgr::GetRandomDungeon(Dungeon::Dungeon& dungeon, uint8 archetype) {
    // Get the index for the archetype ID
    auto& archetypeIndex = m_dungeons.get<Dungeon::DungeonsByArchetype>();
    // Get the range of all dungeons with the specified archetype ID
    auto range = archetypeIndex.equal_range(archetype);
    // Calculate the number of dungeons in the range
    uint32 count = std::distance(range.first, range.second);
    // If there are no dungeons with the specified archetype, return
    if (count == 0) {
        return;
    }
    // Generate a random number within the range of the number of dungeons
    uint32 randomIndex = rand() % count;
    // Get the iterator to the random dungeon
    auto it = range.first;
    std::advance(it, randomIndex);
    // Assign the selected dungeon to the output parameter
    dungeon = *it;
}

void DungeonDataMgr::GetRandomCombatAnomalyDungeon(Dungeon::Dungeon& dungeon, uint8 archetype, float mapSecurityStatus, uint32 ownerFactionID)
{
    if (archetype != Dungeon::Type::Anomaly || ownerFactionID == 0) {
        GetRandomDungeon(dungeon, archetype);
        return;
    }

    auto& archetypeIndex = m_dungeons.get<Dungeon::DungeonsByArchetype>();
    auto range = archetypeIndex.equal_range(archetype);
    const uint32 count = std::distance(range.first, range.second);
    if (count == 0) {
        return;
    }

    std::vector<const Dungeon::Dungeon*> pool;
    pool.reserve(count);
    std::unordered_set<uint32> seen;

    auto collect = [&](bool applySecBand) {
        pool.clear();
        seen.clear();
        for (auto it = range.first; it != range.second; ++it) {
            const Dungeon::Dungeon& d = *it;
            if (!seen.insert(d.dungeonID).second)
                continue;
            if (d.factionID != ownerFactionID)
                continue;
            const uint8 kind = ClassifyAnomalySiteKind(d.name);
            if (applySecBand && !CombatAnomalyAllowedAtMapSec(kind, mapSecurityStatus, IsDenVariantDungeonName(d.name)))
                continue;
            pool.push_back(&d);
        }
    };

    collect(true);
    if (pool.empty()) {
        _log(DUNG__WARNING, "GetRandomCombatAnomalyDungeon - no %u archetype dungeons for faction %u at mapSec %.2f; relaxing sec band filter.",
            (unsigned)archetype, ownerFactionID, mapSecurityStatus);
        collect(false);
    }
    if (pool.empty()) {
        _log(DUNG__WARNING, "GetRandomCombatAnomalyDungeon - no faction %u candidates; full random.", ownerFactionID);
        GetRandomDungeon(dungeon, archetype);
        return;
    }

    dungeon = *pool[static_cast<size_t>(rand() % pool.size())];
}

void DungeonDataMgr::GetDungeon(Dungeon::Dungeon& dungeon, uint32 dungeonID) {
    // Multi-index view by dungeonID
    auto &byDungeonID = m_dungeons.get<Dungeon::DungeonsByID>();

    auto it = byDungeonID.find(dungeonID);
        if (it != byDungeonID.end())
        {
            dungeon = *it;
        } else {
            _log(DUNG__ERROR, "GetDungeon() - Failed to find dungeon with id %u", dungeonID);
        }
}

void DungeonDataMgr::Populate()
{
    // Populate dungeon datasets from DB
    double start = GetTimeMSeconds();
    DBQueryResult *res = new DBQueryResult();
    DBResultRow row;

    // Multi-index view by dungeonID
    auto &byDungeonID = m_dungeons.get<Dungeon::DungeonsByID>();

    DungeonDB::GetAllDungeonData(*res);
    while (res->GetRow(row))
    {
        CreateDungeon(row);
        FillObject(row);
    }

    sLog.Cyan("       DunDataMgr", "%lu Dungeon data sets loaded in %.3fms.",
              byDungeonID.size(), (GetTimeMSeconds() - start));

    //cleanup
    SafeDelete(res);
}

void DungeonDataMgr::CreateDungeon(DBResultRow row) {
    // Multi-index view by dungeonID
    auto &byDungeonID = m_dungeons.get<Dungeon::DungeonsByID>();

    // Check if dungeon already exists for this object
    auto it = byDungeonID.find(row.GetUInt(0));
    if (it != byDungeonID.end()) {
        Dungeon::Dungeon dData = *it;
        // Check if room already exists for this object
        if (dData.rooms.find(row.GetUInt(5)) == dData.rooms.end()) {
            // Create new room
            Dungeon::Room rData;
            rData.roomID = row.GetUInt(5);
            dData.rooms.insert({rData.roomID, rData});

            // Replace record in container
            byDungeonID.erase(dData.dungeonID);
            byDungeonID.insert(dData);
        }
    } else {
        // Create dungeon and room
        Dungeon::Dungeon dData;
        Dungeon::Room rData;
        rData.roomID = row.GetUInt(5);
        dData.rooms.insert({rData.roomID, rData});
        dData.dungeonID = row.GetUInt(0);
        byDungeonID.insert(dData);
    }
}

void DungeonDataMgr::FillObject(DBResultRow row) {
    /*    if (!sDatabase.RunQuery(res, "SELECT dunDungeons.dungeonID "
    "dungeonName, dungeonStatus, factionID, archetypeID, "
    "dunRooms.roomID, dunRooms.roomName, objectID, typeID, groupID, "
    "x, y, z, yaw, pitch, roll, radius "
    "FROM ((dunDungeons "
    "INNER JOIN dunRooms ON dunDungeons.dungeonID = dunRooms.dungeonID) "
    "INNER JOIN dunRoomObjects ON dunRooms.roomID = dunRoomObjects.roomID)"))*/

    // Multi-index view used for inserting
    auto &byDungeonID = m_dungeons.get<Dungeon::DungeonsByID>();

    auto it = byDungeonID.find(row.GetUInt(0));
    if (it != byDungeonID.end()) {
        Dungeon::Dungeon dData = *it;

        // Add the object to the room
        Dungeon::RoomObject oData;
        oData.objectID = row.GetUInt(7);
        oData.typeID = row.GetUInt(8);
        oData.groupID = row.GetUInt(9);
        oData.x = row.GetInt(10);
        oData.y = row.GetInt(11);
        oData.z = row.GetInt(12);
        oData.yaw = row.GetInt(13);
        oData.pitch = row.GetInt(14);
        oData.roll = row.GetInt(15);
        oData.radius = row.GetInt(16);
        dData.rooms[row.GetUInt(5)].objects.push_back(oData);
        // Populate all data for the dungeon and room
        dData.name = row.GetText(1);
        dData.status = row.GetUInt(2);
        dData.factionID = row.GetUInt(3);
        dData.archetypeID = row.GetUInt(4);
        dData.rooms[row.GetUInt(5)].roomID = row.GetUInt(5);
        dData.rooms[row.GetUInt(5)].roomName = row.GetText(6);

        // Replace item in container
        byDungeonID.erase(dData.dungeonID);
        byDungeonID.insert(dData);
    } 
    else {
        _log(DUNG__ERROR, "FillObject() - Failed to find dungeon %u's object in DataMgr. This should never happen.", row.GetUInt(0));
    }
}

const char* DungeonDataMgr::GetDungeonType(int8 typeID)
{
    // Return the string representation of the given dungeon type ID
    switch (typeID) {
        case Dungeon::Type::Mission:        return "Mission";
        case Dungeon::Type::Gravimetric:    return "Gravimetric";
        case Dungeon::Type::Magnetometric:  return "Magnetometric";
        case Dungeon::Type::Radar:          return "Radar";
        case Dungeon::Type::Ladar:          return "Ladar";
        case Dungeon::Type::Rated:          return "Rated";
        case Dungeon::Type::Anomaly:        return "Anomaly";
        case Dungeon::Type::Unrated:        return "Unrated";
        case Dungeon::Type::Escalation:     return "Escalation";
        case Dungeon::Type::Wormhole:       return "Wormhole";
        default:                            return "Invalid";
    }
}


DungeonMgr::DungeonMgr(SystemManager* mgr, EVEServiceManager& svc)
: m_system(mgr),
m_services(svc),
m_anomMgr(nullptr),
m_spawnMgr(nullptr),
m_initalized(false)
{
}

DungeonMgr::~DungeonMgr()
{
    // TODO: clean up and free any resources
}

bool DungeonMgr::Init(AnomalyMgr* anomMgr, SpawnMgr* spawnMgr)
{
    if (!m_initalized)
    {
        m_anomMgr = anomMgr;
        m_spawnMgr = spawnMgr;

        if (m_anomMgr == nullptr) {
            _log(COSMIC_MGR__ERROR, "System Init Fault. anomMgr == nullptr.  Not Initializing Dungeon Manager for %s(%u)", m_system->GetName(), m_system->GetID());
            return m_initalized;
        }

        if (m_spawnMgr == nullptr) {
            _log(COSMIC_MGR__ERROR, "System Init Fault. spawnMgr == nullptr.  Not Initializing Dungeon Manager for %s(%u)", m_system->GetName(), m_system->GetID());
            return m_initalized;
        }

        if (!sConfig.cosmic.DungeonEnabled){
            _log(COSMIC_MGR__INIT, "Dungeon System Disabled.  Not Initializing Dungeon Manager for %s(%u)", m_system->GetName(), m_system->GetID());
            return true;
        }

        m_spawnMgr->SetDungMgr(this);

        _log(COSMIC_MGR__INIT, "DungeonMgr Initialized for %s(%u)", m_system->GetName(), m_system->GetID());

        m_initalized = true;
        return true;
    }
    return false;
}

void DungeonMgr::Process()
{
    // TODO: process and update any active dungeons in the system
    if (!m_initalized)
        return;
}

bool DungeonMgr::MakeDungeon(CosmicSignature& sig, uint32 dungeonID)
{
    Dungeon::Dungeon dData;

    // If we are given a dungeonID, use it otherwise pick a random dungeon based on archetype
    if (dungeonID == 0) {
        if (sig.dungeonType == Dungeon::Type::Anomaly && sig.ownerID != 0)
            sDunDataMgr.GetRandomCombatAnomalyDungeon(dData, sig.dungeonType, m_system->GetMapSecurityStatus(), sig.ownerID);
        else
            sDunDataMgr.GetRandomDungeon(dData, sig.dungeonType);
    } else {
        sDunDataMgr.GetDungeon(dData, dungeonID);
    }

    if (!dData.name.empty())
        sig.sigName = dData.name;

    if (dData.rooms.empty()) {
        _log(COSMIC_MGR__WARNING, "DungeonMgr::MakeDungeon - no dungeon rooms for type %s(%u) in %s(%u) (data dungeonID %u). Seed dunDungeons/dunRooms for this archetype.",
            sDunDataMgr.GetDungeonType(sig.dungeonType), (unsigned)sig.dungeonType, m_system->GetName(), m_system->GetID(), dData.dungeonID);
        return false;
    }

    if ((sig.sigGroupID == EVEDB::invGroups::Cosmic_Signature) || (sig.sigGroupID == EVEDB::invGroups::Cosmic_Anomaly)) {
        // Create a new anomaly inventory item to track entire dungeon under
        ItemData iData(sig.sigTypeID, sig.ownerID, sig.systemID, flagNone, sig.sigName.c_str(), sig.position/*, info*/);

        InventoryItemRef iRef = sItemFactory.SpawnItem(iData);
        if (iRef.get() == nullptr)
            return false;
        iRef->SetCustomInfo(std::string("livedungeon").c_str());
        iRef->SaveItem();

        CelestialSE* cSE = new CelestialSE(iRef, m_system->GetServiceMgr(), m_system);

        if (cSE == nullptr)
            return false;

        // dont add signal thru sysMgr.  signal is added when this returns to anomMgr
        m_system->AddEntity(cSE, false);
        sig.sigItemID = iRef->itemID();
        sig.bubbleID = cSE->SysBubble()->GetID();

        _log(COSMIC_MGR__TRACE, "DungeonMgr::Create() - %s using dungeonID %u", sig.sigName.c_str(), dData.dungeonID);

        // Create the new live dungeon
        Dungeon::LiveDungeon newDungeon;
        newDungeon.anomalyID = iRef->itemID();
        newDungeon.systemID = m_system->GetID();
        newDungeon.dungeonType = (uint8)sig.dungeonType;
        newDungeon.ownerID = sig.ownerID;
        newDungeon.escalationDone = false;
        newDungeon.anomalyWaveMode = false;
        newDungeon.templateFactionID = 0;
        newDungeon.anomalyRegularWaveCount = 0;
        newDungeon.anomalyNextWaveToSpawn = 0;
        newDungeon.anomalyWantCommander = false;
        newDungeon.anomalyCommanderSpawned = false;
        newDungeon.combatAnchor = sig.position;
        newDungeon.anomalySiteKind = ClassifyAnomalySiteKind(dData.name);

        // Iterate through rooms and handle item spawning for each room
        uint16 roomCounter = 0;
        for (auto const& room : dData.rooms) {
            Dungeon::LiveRoom newRoom;
            // Set room position
            // Handle first room differently as it will be at the origin point of signature
            if (roomCounter == 0) {
                newRoom.position = sig.position;
            } else {
                // The following rooms shall be 100M kilometers in x direction from previous room.
                GPoint pos;
                pos.x = newDungeon.rooms[roomCounter - 1].position.x + NEXT_DUNGEON_ROOM_DIST;
                pos.y = newDungeon.rooms[roomCounter - 1].position.y;
                pos.z = newDungeon.rooms[roomCounter - 1].position.z;
                newRoom.position = pos;
            }

            for (auto object : room.second.objects ) {
                GPoint pos;
                // Set position for each object
                pos.x = newRoom.position.x + object.x;
                pos.y = newRoom.position.y + object.y;
                pos.z = newRoom.position.z + object.z;

                // Require invTypes entry, then classify by the type's group (authoritative category).
                // NPC rats use category Entity (see InventoryItem.cpp); only checking Ship+Drone sent every
                // Entity rat to the celestial branch so combat sites had no hostiles on overview.
                if (!sDataMgr.HasType(object.typeID)) {
                    _log(COSMIC_MGR__ERROR, "DungeonMgr::MakeDungeon - invTypes missing typeID %u (%s); cannot spawn room object.", \
                        object.typeID, sig.sigName.c_str());
                    continue;
                }
                Inv::TypeData objType;
                sDataMgr.GetType(object.typeID, objType);
                Inv::GrpData objGroup{};
                const bool groupResolved = sDataMgr.HasGroup(objType.groupID);
                if (groupResolved)
                    sDataMgr.GetGroup(objType.groupID, objGroup);
                uint8 cat = groupResolved ? objGroup.catID : 0;
                bool asNpc = (cat == EVEDB::invCategories::Ship
                    || cat == EVEDB::invCategories::Drone
                    || cat == EVEDB::invCategories::Entity);
                // Missing invGroups row leaves catID 0 and previously sent all objects to the celestial branch.
                if (!asNpc && !groupResolved && objType.groupID) {
                    const uint8 dt = sig.dungeonType;
                    if (dt == Dungeon::Type::Anomaly || dt == Dungeon::Type::Unrated
                        || dt == Dungeon::Type::Rated || dt == Dungeon::Type::Escalation) {
                        _log(COSMIC_MGR__WARNING,
                            "DungeonMgr::MakeDungeon - invGroups missing groupID %u (type %u, %s); using NPC spawn path.",
                            objType.groupID, object.typeID, sig.sigName.c_str());
                        asNpc = true;
                    }
                }
                // Cosmic anomalies use scripted waves from faction asteroid pirate groups instead of dunRoomObjects NPC rows.
                if (asNpc && sig.dungeonType == Dungeon::Type::Anomaly)
                    continue;
                if (asNpc) {
                    // Anomaly sites use random positions; celestials are often outside gate/belt bubbles.
                    // FindBubble() returns nullptr there and no rats spawn — use GetBubble() like BubbleManager::Add().
                    m_spawnMgr->DoSpawnForAnomaly(sBubbleMgr.GetBubble(m_system, pos), pos, GetRandLevel(), object.typeID, newDungeon.anomalyID);
                }
                // Otherwise, spawn as a normal celestial object
                else {
                    // Define ItemData object for each RoomObject
                    ItemData dData(object.typeID, sig.ownerID, sig.systemID, flagNone, sDataMgr.GetTypeName(object.typeID), pos);

                    // Spawn the object (and persist it across system unloads)
                    iRef = sItemFactory.SpawnItem(dData);
                    if (iRef.get() == nullptr) {
                        _log(COSMIC_MGR__ERROR, "DungeonMgr::Create() - Unable to spawn item with type %s for room %u dungeon with anomaly itemID %u", sDataMgr.GetTypeName(object.typeID), roomCounter, newDungeon.anomalyID);
                        return false;
                    }
                    iRef->SetCustomInfo(("livedungeon_" + std::to_string(newDungeon.anomalyID)).c_str());
                    iRef->SaveItem();

                    cSE = new CelestialSE(iRef, m_system->GetServiceMgr(), m_system);
                    m_system->AddEntity(cSE, false);
                    newRoom.items.push_back(iRef->itemID());
                }
            }

            newDungeon.rooms.insert({roomCounter, newRoom});
            roomCounter++;
        }

        // Finally add the new dungeon to the system-wide list for tracking
        m_dungeonList.insert({newDungeon.anomalyID, newDungeon});

        if (sig.dungeonType == Dungeon::Type::Anomaly) {
            auto dit = m_dungeonList.find(newDungeon.anomalyID);
            if (dit != m_dungeonList.end()) {
                Dungeon::LiveDungeon& ld = dit->second;
                ld.anomalyWaveMode = true;
                ld.templateFactionID = dData.factionID;
                ld.combatAnchor = sig.position;
                uint8 cap = sConfig.exploring.AnomalyCombatWaves;
                if (cap < 1)
                    cap = 1;
                if (cap > 12)
                    cap = 12;
                const WaveSpec* profTable = nullptr;
                uint8 profileWaves = 0;
                if (GetWaveTable(ld.anomalySiteKind, profTable, profileWaves) && profileWaves > 0)
                    ld.anomalyRegularWaveCount = std::min(profileWaves, cap);
                else {
                    ld.anomalyRegularWaveCount = std::min(static_cast<uint8>(5), cap);
                }
                ld.anomalyWantCommander = (MakeRandomFloat() < sConfig.exploring.AnomalyCommanderChance);
                ld.anomalyCommanderSpawned = false;
                ld.anomalyNextWaveToSpawn = 2;
                SpawnAnomalyWaveRats(ld, 1);
                _log(COSMIC_MGR__MESSAGE, "DungeonMgr::MakeDungeon - anomaly waves for %s: siteKind=%u regular=%u commander_roll=%s (faction %u).",
                    sig.sigName.c_str(), (unsigned)ld.anomalySiteKind, (unsigned)ld.anomalyRegularWaveCount, ld.anomalyWantCommander ? "yes" : "no", dData.factionID);
            }
        }
        return true;
    }
    return false;
}

void DungeonMgr::SpawnAnomalyTypeNearAnchor(Dungeon::LiveDungeon& ld, uint16 groupID, uint8 count, bool commanderWave)
{
    if (!m_spawnMgr || !groupID || !count)
        return;
    const uint8 level = GetRandLevel();
    for (uint8 i = 0; i < count; ++i) {
        const uint16 tid = sDataMgr.GetRandomPublishedTypeInGroup(groupID);
        if (!tid) {
            _log(COSMIC_MGR__WARNING, "SpawnAnomalyTypeNearAnchor - no published types in group %u (anomaly %u).", groupID, ld.anomalyID);
            continue;
        }
        GPoint p(ld.combatAnchor);
        p.MakeRandomPointOnSphere(4000.0 + static_cast<double>(rand() % 12000));
        m_spawnMgr->DoSpawnForAnomaly(sBubbleMgr.GetBubble(m_system, p), p, level, tid, ld.anomalyID, commanderWave);
    }
}

void DungeonMgr::SpawnAnomalyWaveRats(Dungeon::LiveDungeon& ld, uint8 waveIndex)
{
    RunAnomalyProfileWave(this, ld, waveIndex);
}

void DungeonMgr::SpawnAnomalyCommanderRats(Dungeon::LiveDungeon& ld)
{
    AnomPirateGroups pg{};
    GetAnomPirateGroups(ld.templateFactionID, pg);
    if (ld.anomalySiteKind == Dungeon::AnomalySiteKind::Unknown) {
        _log(COSMIC_MGR__MESSAGE, "DungeonMgr::SpawnAnomalyCommanderRats - commander wave for anomaly %u (legacy)", ld.anomalyID);
        if ((rand() % 2) != 0) {
            SpawnAnomalyTypeNearAnchor(ld, pg.cmdBattleship, 1, true);
            if ((rand() % 3) == 0)
                SpawnAnomalyTypeNearAnchor(ld, pg.cmdFrigate, 1, true);
        } else {
            SpawnAnomalyTypeNearAnchor(ld, pg.cmdCruiser, static_cast<uint8>(1 + (rand() % 2)), true);
            SpawnAnomalyTypeNearAnchor(ld, pg.cmdFrigate, 1, true);
        }
        return;
    }
    SpawnAnomalyCommanderRatsTiered(this, ld, pg);
}

bool DungeonMgr::TrySpawnNextAnomalyWave(uint32 dungeonAnomalyItemID)
{
    if (!m_initalized || m_spawnMgr == nullptr)
        return false;
    auto it = m_dungeonList.find(dungeonAnomalyItemID);
    if (it == m_dungeonList.end())
        return false;
    Dungeon::LiveDungeon& ld = it->second;
    if (!ld.anomalyWaveMode)
        return false;

    if (ld.anomalyNextWaveToSpawn <= ld.anomalyRegularWaveCount) {
        SpawnAnomalyWaveRats(ld, ld.anomalyNextWaveToSpawn);
        ++ld.anomalyNextWaveToSpawn;
        return true;
    }
    if (ld.anomalyWantCommander && !ld.anomalyCommanderSpawned) {
        ld.anomalyCommanderSpawned = true;
        SpawnAnomalyCommanderRats(ld);
        return true;
    }
    return false;
}

void DungeonMgr::OnDungeonCombatCleared(uint32 dungeonAnomalyItemID)
{
    if (!m_initalized || m_anomMgr == nullptr)
        return;

    auto it = m_dungeonList.find(dungeonAnomalyItemID);
    if (it == m_dungeonList.end())
        return;

    Dungeon::LiveDungeon& ld = it->second;

    const uint8 t = ld.dungeonType;
    if (t != Dungeon::Type::Anomaly && t != Dungeon::Type::Unrated)
        return;

    if (!ld.escalationDone) {
        float p = sConfig.exploring.EscalationChance;
        if (p < 0.f)
            p = 0.f;
        if (p > 1.f)
            p = 1.f;
        if (MakeRandomFloat() <= p) {
            GPoint anchor(NULL_ORIGIN);
            if (!ld.rooms.empty())
                anchor = ld.rooms.begin()->second.position;
            m_anomMgr->SpawnEscalationAt(anchor, ld.ownerID);
        }
        ld.escalationDone = true;
    }

    RemoveCompletedDungeonSite(dungeonAnomalyItemID);
}

void DungeonMgr::RemoveCompletedDungeonSite(uint32 dungeonAnomalyItemID)
{
    if (!m_initalized || m_system == nullptr)
        return;

    auto it = m_dungeonList.find(dungeonAnomalyItemID);
    if (it == m_dungeonList.end())
        return;

    Dungeon::LiveDungeon& ld = it->second;
    for (auto& roomPair : ld.rooms) {
        for (uint32 itemID : roomPair.second.items) {
            if (itemID == 0 || itemID == dungeonAnomalyItemID)
                continue;
            SystemEntity* se = m_system->GetSE(itemID);
            if (se != nullptr)
                se->Delete();
        }
    }

    SystemEntity* anchor = m_system->GetSE(dungeonAnomalyItemID);
    if (anchor != nullptr)
        anchor->Delete();

    m_dungeonList.erase(dungeonAnomalyItemID);
    m_db.DeleteSignatureBySigItemID(dungeonAnomalyItemID);
}

int8 DungeonMgr::GetFaction(uint32 factionID)
{
    switch (factionID) {
        case factionAngel:          return 2;
        case factionSanshas:        return 5;
        case factionGuristas:       return 4;
        case factionSerpentis:      return 1;
        case factionBloodRaider:    return 3;
        case factionRogueDrones:    return 6;
        case 0:                     return 7;
        // these are incomplete.  set to default (region rat)
        case factionAmarr:
        case factionAmmatar:
        case factionCaldari:
        case factionGallente:
        case factionMinmatar:
        default:
            return GetFaction(sDataMgr.GetRegionRatFaction(m_system->GetRegionID()));
    }
}

int8 DungeonMgr::GetRandLevel()
{
    double level = MakeRandomFloat();
    _log(DUNG__TRACE, "DungeonMgr::GetRandLevel() - level = %.2f", level);

    if (level < 0.15) {
        return 4;
    } else if (level < 0.25) {
        return 3;
    } else if (level < 0.50) {
        return 2;
    } else {
        return 1;
    }
}
