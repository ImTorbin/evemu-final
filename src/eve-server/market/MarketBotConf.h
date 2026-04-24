
 /**
  * @name MarketBotConf.h
  *   system for automating/emulating buy and sell orders on the market.
  * idea and some code taken from AuctionHouseBot - Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
  * @Author:         Allan
  * @date:   10 August 2016
  */

#ifndef EVEMU_MARKET_BOTCONFIG_H_
#define EVEMU_MARKET_BOTCONFIG_H_

#include <vector>

#include "../eve-server.h"
#include "../../eve-core/utils/XMLParserEx.h"


class MarketBotConf
:public XMLParserEx,
 public Singleton< MarketBotConf >
{
public:
    MarketBotConf();
    ~MarketBotConf() { /* do nothing here */ }

    // From <main/>
    struct
    {
        uint8 DataRefreshTime;
        uint8 OrderLifetime;
        uint32 MaxISKPerOrder;
    } main;

    // Empire trade hub stationIDs (from <hubs><station id="..."/></hubs>)
    struct
    {
        std::vector<uint32> stationIDs;
    } hubs;

    // From <buy/>
    struct
    {
        uint32 HubBuyOrdersPerRefresh;
        uint32 SprinkleSystemsCount;
        uint32 SprinkleBuyOrdersPerRefresh;
        float HubBuyPriceMinMult;
        float HubBuyPriceMaxMult;
        float SprinkleBuyPriceMinMult;
        float SprinkleBuyPriceMaxMult;
        uint8 MinBuyAmount;
    } buy;

    // From <sell/>
    struct
    {
        bool SellNamedItem;
        uint32 HubSellOrdersPerRefresh;
        uint32 SprinkleSellOrdersPerRefresh;
        uint8 SellItemMetaLevelMin;
        uint8 SellItemMetaLevelMax;
        uint8 MinSellAmount;
    } sell;

protected:
    bool ProcessBotConf( const TiXmlElement* ele );
    bool ProcessMain( const TiXmlElement* ele );
    bool ProcessHubs( const TiXmlElement* ele );
    bool ProcessBuy( const TiXmlElement* ele );
    bool ProcessSell( const TiXmlElement* ele );
};


#define sMBotConf \
   (MarketBotConf::get())

#endif  // EVEMU_MARKET_BOTCONFIG_H_
