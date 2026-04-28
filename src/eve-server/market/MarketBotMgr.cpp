/**
  * @name MarketBotMgr.h
  *   system for automating/emulating buy and sell orders on the market.
  * idea and some code taken from AuctionHouseBot - Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
  * @Author:         Allan
  * @date:   10 August 2016
  * @version:  0.15 (config version)
  */

#include "eve-server.h"
#include "EVEServerConfig.h"
#include "market/MarketBotConf.h"
#include "market/MarketBotMgr.h"
#include "market/MarketMgr.h"
#include "market/MarketProxyService.h"

// ---marketbot update; everything past this point has been completely changed.
#include "market/MarketDB.h"
#include "inventory/ItemType.h"
#include "inventory/ItemFactory.h"
#include "inventory/InventoryItem.h"
#include "StaticDataMgr.h"
#include "station/StationDataMgr.h"
#include "system/SystemManager.h"
#include "system/SystemEntity.h"
#include <random>
#include <cstdint>
#include <chrono>

extern SystemManager* sSystemMgr;

static constexpr int64 FILETIME_TICKS_PER_DAY = 864000000000;  // 100ns ticks per day; for expelorders to be removed prematurally

static const uint32 MARKETBOT_MAX_ITEM_ID = 30000;
static const std::vector<uint32> VALID_GROUPS = {
    // Ores & Mining
    18,                                    // Minerals
    450, 451, 452, 453, 454, 455, 456,     // Raw ores (part 1)
    457, 458, 459, 460, 461, 462,
    465, 466, 467, 468, 469,              // Raw ores (part 2)
    479,                                  // Scanner Probe
    482,                                  // Mining Crystals
    492,                                  // Survey Probe
    538,                                  // Data Miners
    548,                                  // Interdiction Probe
    663,                                  // Mercoxit Mining Crystals

    // Ammo / Charges
    83,                                   // Projectile Ammo
    84,                                   // Missiles
    85,                                   // Hybrid Charges
    86,                                   // Frequency Crystals
    87,                                   // Cap Booster Charges
    88,                                   // Defender Missiles
    89,                                   // Torpedoes
    90,                                   // Bombs
    92,                                   // Mines
    372, 373, 374, 375, 376, 377,         // Advanced Ammo
    384, 385, 386, 387, 388, 389,         // Extended Missiles (part 1)
    390, 391, 392, 393, 394, 395, 396,    // Extended Missiles (part 2)
    648,                                  // Advanced Rocket
    653, 654, 655, 656, 657,              // Advanced Missiles
    772,                                  // Assault Missiles

    // Boosters for Implants
    303
};

static constexpr uint32 BOT_OWNER_ID = 1000125; // NPC corp owner, default CONCORD

// helper random generators
int GetRandomInt(int min, int max) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

float GetRandomFloat(float min, float max) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

MarketBotDataMgr::MarketBotDataMgr() {
    m_initalized = false;
}

int MarketBotDataMgr::Initialize() {
    m_initalized = true;
    sLog.Blue(" MarketBotDataMgr", "Market Bot Data Manager Initialized."); // load current data
    return 1;
}

MarketBotMgr::MarketBotMgr()
: m_nextRunTime{},
  m_initalized(false),
  m_tradeHubsResolved(false)
{
}

int MarketBotMgr::Initialize() {
    if (!sMBotConf.ParseFile(sConfig.files.marketBotSettings.c_str())) {
        sLog.Error("       ServerInit", "Loading Market Bot Config file '%s' failed.", sConfig.files.marketBotSettings.c_str());
        return 0;
    }

    m_initalized = true;
    sMktBotDataMgr.Initialize();

    m_nextRunTime = Clock::now() + std::chrono::minutes(sMBotConf.main.DataRefreshTime);
    sLog.Cyan("     MarketBotMgr", "Timer initialized. First automated cycle will run in %d minutes.", sMBotConf.main.DataRefreshTime);

    sLog.Blue("     MarketBotMgr", "Market Bot Manager Initialized.");
    return 1;
}

// Called on minute tick from EntityList
void MarketBotMgr::Process(bool overrideTimer) {
    TimePoint now = Clock::now();

    sLog.Green("     MarketBotMgr", "MarketBot Process() invoked on tick.");
    codelog(MARKET__TRACE, "MarketBot Process() invoked on tick.");

    sLog.Green("     MarketBotMgr", "Entered MarketBotMgr::Process()\n");
    codelog(MARKET__TRACE, ">> Entered MarketBotMgr::Process()");

    if (!m_initalized) {
        sLog.Error("     MarketBotMgr", "MarketBotMgr not initialized ù skipping run\n");
        codelog(MARKET__ERROR, "Process() called but MarketBotMgr is not initialized.");
        return;
    }

    if (!overrideTimer && now + std::chrono::seconds(5) < m_nextRunTime) { // ---marketbot update; 5 second jitter
        auto timeLeft = std::chrono::duration_cast<std::chrono::milliseconds>(m_nextRunTime - now).count();
        if (timeLeft > 0) {
            sLog.Green("     Trader Joe", "Update timer not ready yet. Next run in %lld seconds.", timeLeft / 1000);
            codelog(MARKET__TRACE, "Trader Joe waiting ù next run in %lld seconds.", timeLeft);
            return;
        }
    }

    ResolveTradeHubsIfNeeded();

    int totalBuyOrders = 0;
    int totalSellOrders = 0;
    const int expiredOrders = ExpireOldOrders();

    for (const ResolvedHub& hub : m_tradeHubs) {
        sLog.Green("     Market Bot Mgr", "Trader Joe hub pass: station %u (system %u)", hub.stationID, hub.solarSystemID);
        totalBuyOrders += PlaceBuyOrdersAtHub(hub);
        totalSellOrders += PlaceSellOrdersAtHub(hub);
    }

    const std::vector<uint32> sprinkleSystems = GetSprinkleSystems();
    sLog.Green("     Market Bot Mgr", "Trader Joe sprinkle pass: %zu random systems.", sprinkleSystems.size());
    for (uint32 systemID : sprinkleSystems) {
        sLog.Green("     Market Bot Mgr", "Trader Joe placing sprinkle orders in systemID: %u", systemID);
        totalBuyOrders += PlaceBuyOrdersSprinkle(systemID);
        totalSellOrders += PlaceSellOrdersSprinkle(systemID);
    }

    sLog.Green("     Trader Joe", "Master Summary: Created %d buy orders and %d sell orders (%zu hubs, %zu sprinkle systems). Removed %d old orders.",
        totalBuyOrders, totalSellOrders, m_tradeHubs.size(), sprinkleSystems.size(), expiredOrders);

    codelog(MARKET__TRACE, "Trader Joe Master Summary: Created %d buy orders and %d sell orders. Removed %d old orders.",
        totalBuyOrders, totalSellOrders, expiredOrders);

    sLog.Green("     Trader Joe", "Cycle complete. Resetting timer.");
    codelog(MARKET__TRACE, "Trader Joe cycle complete. Resetting timer.");
    m_nextRunTime = Clock::now() + std::chrono::minutes(sMBotConf.main.DataRefreshTime);
    sLog.Green("     Trader Joe", "Timer reset. Next run in %d minutes.", sMBotConf.main.DataRefreshTime);
}

void MarketBotMgr::ForceRun(bool resetTimer) {
    sLog.Warning("     ForceRun", "Manually starting Trader Joe.");

    if (!m_initalized) {
        sLog.Yellow("     Trader Joe", "MarketBotMgr not initialized ù skipping run.");
        return;
    }

    sLog.Green("     Trader Joe", "Running Process() now...");
    this->Process(true);  // force override
    sLog.Green("     Trader Joe", "Finished Process().");
    
    if (resetTimer) {
        m_nextRunTime = Clock::now() + std::chrono::minutes(sMBotConf.main.DataRefreshTime);
        sLog.Green("     Trader Joe", "Timer reset. Next run in %d minutes.", sMBotConf.main.DataRefreshTime);
    }
}

void MarketBotMgr::AddSystem() { /* To be implemented if needed */ }
void MarketBotMgr::RemoveSystem() { /* To be implemented if needed */ }

int MarketBotMgr::ExpireOldOrders() {
    uint64_t now = GetFileTimeNow();

    DBQueryResult res;
    DBResultRow row;

    int expiredCount = 0;

    sLog.Yellow("     Trader Joe", "ExpireOldOrders: now = %" PRIu64, now);
    codelog(MARKET__TRACE, "ExpireOldOrders: now = %" PRIu64, now);

    if (!sDatabase.RunQuery(res,
        "SELECT orderID FROM mktOrders WHERE (issued + CAST(duration AS UNSIGNED) * %" PRIu64 ") < CAST(%" PRIu64 " AS UNSIGNED) AND ownerID = %u AND volEntered != 550",
        FILETIME_TICKS_PER_DAY, now, BOT_OWNER_ID)) {
        codelog(MARKET__DB_ERROR, "Failed to query expired bot orders.");
        return 0;
    }

    while (res.GetRow(row)) {
        uint32 orderID = row.GetUInt(0);
        MarketDB::DeleteOrder(orderID);
        ++expiredCount;
        codelog(MARKET__TRACE, "Expired Trader Joe order %u", orderID);
    }

    return expiredCount;
}

void MarketBotMgr::ResolveTradeHubsIfNeeded() {
    if (m_tradeHubsResolved)
        return;
    m_tradeHubsResolved = true;
    m_tradeHubs.clear();

    std::vector<uint32> ids = sMBotConf.hubs.stationIDs;
    if (ids.empty()) {
        ids = { 60003760, 60011866, 60008494, 60005686 }; // Jita, Dodixie, Amarr, Hek (classic empire hubs)
        sLog.Warning("     MarketBotMgr", "No <hubs> in MarketBot.xml ù using default empire trade hub station IDs.");
    }

    for (uint32 sid : ids) {
        if (!sDataMgr.IsStation(sid)) {
            sLog.Error("     MarketBotMgr", "Trade hub stationID %u is not in static data ù skipping.", sid);
            continue;
        }
        ResolvedHub hub {};
        hub.stationID = sid;
        hub.solarSystemID = sDataMgr.GetStationSystem(sid);
        hub.regionID = sDataMgr.GetStationRegion(sid);
        m_tradeHubs.push_back(hub);
        sLog.Cyan("     MarketBotMgr", "Trade hub: station %u, system %u, region %u.", hub.stationID, hub.solarSystemID, hub.regionID);
    }

    if (m_tradeHubs.empty())
        sLog.Error("     MarketBotMgr", "No valid trade hubs resolved ù only sprinkle orders will be created.");
}

int MarketBotMgr::PlaceBuyOrdersAtHub(const ResolvedHub& hub) {
    int orderCount = 0;
    for (uint32 n = 0; n < sMBotConf.buy.HubBuyOrdersPerRefresh; ++n) {
        uint32 itemID = SelectRandomItemID();
        const ItemType* type = sItemFactory.GetType(itemID);
        if (!type)
            continue;

        uint32 quantity = GetRandomQuantity(type->groupID());
        double price = CalculateBuyPrice(itemID, sMBotConf.buy.HubBuyPriceMinMult, sMBotConf.buy.HubBuyPriceMaxMult);

        if (price * quantity > sMBotConf.main.MaxISKPerOrder) {
            if (quantity > 1) {
                quantity = 1;
                if (price > sMBotConf.main.MaxISKPerOrder) {
                    codelog(MARKET__TRACE, "Skipping itemID %u due to price %.2f ISK exceeding MaxISKPerOrder.", itemID, price);
                    continue;
                }
            } else {
                codelog(MARKET__TRACE, "Skipping itemID %u even at quantity = 1 due to price %.2f ISK", itemID, price);
                continue;
            }
        }

        const double escrow = price * quantity;

        Market::SaveData order {};
        order.typeID = itemID;
        order.regionID = hub.regionID;
        order.stationID = hub.stationID;
        order.solarSystemID = hub.solarSystemID;
        order.minVolume = 1;
        order.volEntered = quantity;
        order.volRemaining = quantity;
        order.price = static_cast<float>(price);
        order.escrow = static_cast<float>(escrow);
        order.duration = sMBotConf.main.OrderLifetime;
        order.bid = true;
        order.contraband = false;
        order.jumps = 1;
        order.issued = GetFileTimeNow();
        order.isCorp = false;
        order.ownerID = BOT_OWNER_ID;
        order.orderRange = 32767;
        order.memberID = 0;
        order.accountKey = 1000;

        if (MarketDB::StoreOrder(order)) {
            ++orderCount;
            codelog(MARKET__TRACE, "BUY order created for typeID %u, qty %u, price %.2f ISK, hub station %u",
                order.typeID, order.volEntered, order.price, order.stationID);
        } else {
            codelog(MARKET__ERROR, "Failed to store BUY order for typeID %u at station %u", order.typeID, order.stationID);
        }
    }
    return orderCount;
}

int MarketBotMgr::PlaceSellOrdersAtHub(const ResolvedHub& hub) {
    int orderCount = 0;
    for (uint32 n = 0; n < sMBotConf.sell.HubSellOrdersPerRefresh; ++n) {
        uint32 itemID = SelectRandomItemID();
        const ItemType* type = sItemFactory.GetType(itemID);
        if (!type)
            continue;

        uint32 quantity = GetRandomQuantity(type->groupID());
        double price = CalculateSellPrice(itemID);

        if (price * quantity > sMBotConf.main.MaxISKPerOrder) {
            if (quantity > 1) {
                quantity = 1;
                if (price > sMBotConf.main.MaxISKPerOrder) {
                    codelog(MARKET__TRACE, "Skipping itemID %u due to price %.2f ISK exceeding MaxISKPerOrder.", itemID, price);
                    continue;
                }
            } else {
                codelog(MARKET__TRACE, "Skipping itemID %u even at quantity = 1 due to price %.2f ISK", itemID, price);
                continue;
            }
        }

        Market::SaveData order {};
        order.typeID = itemID;
        order.regionID = hub.regionID;
        order.stationID = hub.stationID;
        order.solarSystemID = hub.solarSystemID;
        order.minVolume = 1;
        order.volEntered = quantity;
        order.volRemaining = quantity;
        order.price = static_cast<float>(price);
        order.escrow = 0;
        order.duration = sMBotConf.main.OrderLifetime;
        order.bid = false;
        order.contraband = false;
        order.jumps = 1;
        order.issued = GetFileTimeNow();
        order.isCorp = false;
        order.ownerID = BOT_OWNER_ID;
        order.orderRange = -1;
        order.memberID = 0;
        order.accountKey = 1000;

        if (MarketDB::StoreOrder(order)) {
            ++orderCount;
            codelog(MARKET__TRACE, "SELL order created for typeID %u at hub station %u", order.typeID, order.stationID);
        } else {
            codelog(MARKET__ERROR, "Failed to store SELL order for typeID %u at station %u", order.typeID, order.stationID);
        }
    }
    return orderCount;
}

int MarketBotMgr::PlaceBuyOrdersSprinkle(uint32 systemID) {
    SystemData sysData {};
    if (!sDataMgr.GetSystemData(systemID, sysData)) {
        codelog(MARKET__ERROR, "Failed to get system data for system %u", systemID);
        return 0;
    }

    std::vector<uint32> availableStations;
    if (!sDataMgr.GetStationListForSystem(systemID, availableStations) || availableStations.empty()) {
        codelog(MARKET__ERROR, "No stations found for system %u", systemID);
        return 0;
    }

    int orderCount = 0;
    for (uint32 n = 0; n < sMBotConf.buy.SprinkleBuyOrdersPerRefresh; ++n) {
        const uint32 stationID = availableStations[static_cast<size_t>(GetRandomInt(0, static_cast<int>(availableStations.size()) - 1))];
        uint32 itemID = SelectRandomItemID();
        const ItemType* type = sItemFactory.GetType(itemID);
        if (!type)
            continue;

        uint32 quantity = GetRandomQuantity(type->groupID());
        double price = CalculateBuyPrice(itemID, sMBotConf.buy.SprinkleBuyPriceMinMult, sMBotConf.buy.SprinkleBuyPriceMaxMult);

        if (price * quantity > sMBotConf.main.MaxISKPerOrder) {
            if (quantity > 1) {
                quantity = 1;
                if (price > sMBotConf.main.MaxISKPerOrder) {
                    codelog(MARKET__TRACE, "Skipping itemID %u due to price %.2f ISK exceeding MaxISKPerOrder.", itemID, price);
                    continue;
                }
            } else {
                codelog(MARKET__TRACE, "Skipping itemID %u even at quantity = 1 due to price %.2f ISK", itemID, price);
                continue;
            }
        }

        const double escrow = price * quantity;

        Market::SaveData order {};
        order.typeID = itemID;
        order.regionID = sysData.regionID;
        order.stationID = stationID;
        order.solarSystemID = systemID;
        order.minVolume = 1;
        order.volEntered = quantity;
        order.volRemaining = quantity;
        order.price = static_cast<float>(price);
        order.escrow = static_cast<float>(escrow);
        order.duration = sMBotConf.main.OrderLifetime;
        order.bid = true;
        order.contraband = false;
        order.jumps = 1;
        order.issued = GetFileTimeNow();
        order.isCorp = false;
        order.ownerID = BOT_OWNER_ID;
        order.orderRange = 32767;
        order.memberID = 0;
        order.accountKey = 1000;

        if (MarketDB::StoreOrder(order))
            ++orderCount;
    }
    return orderCount;
}

int MarketBotMgr::PlaceSellOrdersSprinkle(uint32 systemID) {
    SystemData sysData {};
    if (!sDataMgr.GetSystemData(systemID, sysData)) {
        codelog(MARKET__ERROR, "Trader Joe: Failed to get system data for system %u", systemID);
        return 0;
    }

    std::vector<uint32> availableStations;
    if (!sDataMgr.GetStationListForSystem(systemID, availableStations) || availableStations.empty()) {
        codelog(MARKET__ERROR, "Trader Joe: No stations found for system %u", systemID);
        return 0;
    }

    int orderCount = 0;
    for (uint32 n = 0; n < sMBotConf.sell.SprinkleSellOrdersPerRefresh; ++n) {
        const uint32 stationID = availableStations[static_cast<size_t>(GetRandomInt(0, static_cast<int>(availableStations.size()) - 1))];
        uint32 itemID = SelectRandomItemID();
        const ItemType* type = sItemFactory.GetType(itemID);
        if (!type)
            continue;

        uint32 quantity = GetRandomQuantity(type->groupID());
        double price = CalculateSellPrice(itemID);

        if (price * quantity > sMBotConf.main.MaxISKPerOrder) {
            if (quantity > 1) {
                quantity = 1;
                if (price > sMBotConf.main.MaxISKPerOrder) {
                    codelog(MARKET__TRACE, "Skipping itemID %u due to price %.2f ISK exceeding MaxISKPerOrder.", itemID, price);
                    continue;
                }
            } else {
                codelog(MARKET__TRACE, "Skipping itemID %u even at quantity = 1 due to price %.2f ISK", itemID, price);
                continue;
            }
        }

        Market::SaveData order {};
        order.typeID = itemID;
        order.regionID = sysData.regionID;
        order.stationID = stationID;
        order.solarSystemID = systemID;
        order.minVolume = 1;
        order.volEntered = quantity;
        order.volRemaining = quantity;
        order.price = static_cast<float>(price);
        order.escrow = 0;
        order.duration = sMBotConf.main.OrderLifetime;
        order.bid = false;
        order.contraband = false;
        order.jumps = 1;
        order.issued = GetFileTimeNow();
        order.isCorp = false;
        order.ownerID = BOT_OWNER_ID;
        order.orderRange = -1;
        order.memberID = 0;
        order.accountKey = 1000;

        if (MarketDB::StoreOrder(order))
            ++orderCount;
    }
    return orderCount;
}

std::vector<uint32> MarketBotMgr::GetSprinkleSystems() {
    std::vector<uint32> systemIDs;
    const uint32 count = sMBotConf.buy.SprinkleSystemsCount;
    if (count == 0)
        return systemIDs;
    // Sprinkle orders need a station; random k-space includes WH (31xxxxxx) and empty systems.
    sDataMgr.GetRandomSystemIDsWithStations(count, systemIDs);
    codelog(MARKET__TRACE, "GetSprinkleSystems(): %zu systems with stations", systemIDs.size());
    return systemIDs;
}

uint32 MarketBotMgr::SelectRandomItemID() {
    uint32 itemID = 0;
    const ItemType* type = nullptr;
    uint32 tries = 0;

    do {
        ++tries;
        itemID = GetRandomInt(10, MARKETBOT_MAX_ITEM_ID);
        type = sItemFactory.GetType(itemID);

        if (type && std::find(VALID_GROUPS.begin(), VALID_GROUPS.end(), type->groupID()) != VALID_GROUPS.end()) {
            codelog(MARKET__TRACE, "Selected itemID %u after %u attempts", itemID, tries);
            return itemID;
        }
    } while (tries < 50);

    // If we fail after 50 attempts, log a warning and return fallback value
    codelog(MARKET__WARNING, "Failed to select valid itemID after %u attempts. Returning fallback itemID = 34 (Tritanium)", tries);
    return 34;  // Tritanium, as a safe default
}

uint32 MarketBotMgr::GetRandomQuantity(uint32 groupID) {
    // Large-quantity bulk groups: minerals, ammo, ores, charges, etc.
    if (
        groupID == 18 ||                      // Minerals
        (groupID >= 83 && groupID <= 92) ||   // Basic ammo/charges
        (groupID >= 372 && groupID <= 377) || // Advanced ammo
        (groupID >= 384 && groupID <= 396) || // More missiles
        groupID == 479 ||                     // Scanner Probes
        groupID == 482 ||                     // Mining Crystals
        groupID == 492 ||                     // Survey Probes
        groupID == 538 ||                     // Data Miners
        groupID == 548 ||                     // Interdiction Probe
        groupID == 648 ||                     // Advanced Rocket
        (groupID >= 653 && groupID <= 657) || // Advanced Missiles
        groupID == 663 ||                     // Mercoxit Mining Crystals
        groupID == 772 ||                     // Assault Missiles
        (groupID >= 450 && groupID <= 462) || // Raw ores (part 1)
        (groupID >= 465 && groupID <= 469)    // Raw ores (part 2)
    ) {
        return GetRandomInt(1000, 1000000);  // Large stack sizes
    }

    // Medium-volume: modules, rigs, etc.; way to many to list... disabled for the time being
    // leaving below as an example.
    /*if (
        groupID == 62 ||  // Armor Repairers
        groupID == 63 ||  // Hull Repair
        groupID == 205    // Heat Sink
    ) {
        return GetRandomInt(10, 100);
    }*/

    // Fallback for anything else
    return GetRandomInt(10, 500);
}

double MarketBotMgr::CalculateBuyPrice(uint32 itemID, float priceMinMult, float priceMaxMult) {
    const ItemType* type = sItemFactory.GetType(itemID);
    return type ? type->basePrice() * GetRandomFloat(priceMinMult, priceMaxMult) : 1000.0;
}

double MarketBotMgr::CalculateSellPrice(uint32 itemID) {
    const ItemType* type = sItemFactory.GetType(itemID);
    return type ? type->basePrice() * GetRandomFloat(1.0f, 1.3f) : 1000.0;
}
