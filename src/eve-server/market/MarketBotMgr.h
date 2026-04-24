
 /**
  * @name MarketBotMgr.h
  *   system for automating/emulating buy and sell orders on the market.
  * idea and some code taken from AuctionHouseBot - Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
  * @Author:         Allan
  * @date:   10 August 2016
  * @version:  0.15 (config version)
  */


#ifndef EVEMU_MARKET_MARKETBOTMGR_H_
#define EVEMU_MARKET_MARKETBOTMGR_H_

// ---marketbot update; this has to be declared first.
// ---marketbot update; issue with global timers, will have to fix later; this makes marketbot timer self contained.
#include <chrono>

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
// ---marketbot update

#include "eve-compat.h"
#include "eve-common.h"
#include "utils/Singleton.h"

#include <vector>

class MarketBotDataMgr
: public Singleton<MarketBotDataMgr>
{
public:
    MarketBotDataMgr();
    ~MarketBotDataMgr() { /* do nothing here */ }

    int Initialize();

private:
    bool m_initalized;
};

//Singleton
#define sMktBotDataMgr \
( MarketBotDataMgr::get() )

class MarketBotMgr
: public Singleton<MarketBotMgr>
{
public:
    MarketBotMgr();
    ~MarketBotMgr() { /* do nothing here */ }

    int Initialize();
    void Process(bool overrideTimer = false);

    void AddSystem();
    void RemoveSystem();

    int ExpireOldOrders();

    std::vector<uint32> GetSprinkleSystems();
    uint32 SelectRandomItemID();
    uint32 GetRandomQuantity(uint32 groupID);
    double CalculateSellPrice(uint32 itemID);

    void ForceRun(bool resetTimer = true); // debug command to force MarketBot to run first cycle to generate NPC buy and sell orders.

private:
    struct ResolvedHub {
        uint32 stationID;
        uint32 solarSystemID;
        uint32 regionID;
    };

    void ResolveTradeHubsIfNeeded();
    int PlaceBuyOrdersAtHub(const ResolvedHub& hub);
    int PlaceSellOrdersAtHub(const ResolvedHub& hub);
    int PlaceBuyOrdersSprinkle(uint32 systemID);
    int PlaceSellOrdersSprinkle(uint32 systemID);

    double CalculateBuyPrice(uint32 itemID, float priceMinMult, float priceMaxMult);

    TimePoint m_nextRunTime;
    bool m_initalized;
    bool m_tradeHubsResolved;
    std::vector<ResolvedHub> m_tradeHubs;
};

//Singleton
#define sMktBotMgr \
( MarketBotMgr::get() )

#endif  // EVEMU_MARKET_MARKETBOTMGR_H_