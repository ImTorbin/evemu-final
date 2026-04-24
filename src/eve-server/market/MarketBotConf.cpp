
/**
 * @name MarketBotConf.h
 *   system for automating/emulating buy and sell orders on the market.
 * base config code taken from EVEServerConfig
 * idea and some code taken from AuctionHouseBot - Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * @Author:         Allan
 * @date:   10 August 2016
 * @version:  0.15
 */



#include <cstring>
#include <cstdlib>

#include "market/MarketBotConf.h"


MarketBotConf::MarketBotConf()
{
    // register needed parsers
    AddMemberParser( "marketBot", &MarketBotConf::ProcessBotConf );

    // main
    main.DataRefreshTime = 15;
    main.OrderLifetime = 5;
    main.MaxISKPerOrder = 1500000000;

    // buy (hub-heavy liquidity + light sprinkle elsewhere)
    buy.HubBuyOrdersPerRefresh = 120;
    buy.SprinkleSystemsCount = 5;
    buy.SprinkleBuyOrdersPerRefresh = 10;
    buy.HubBuyPriceMinMult = 0.95f;
    buy.HubBuyPriceMaxMult = 1.15f;
    buy.SprinkleBuyPriceMinMult = 0.80f;
    buy.SprinkleBuyPriceMaxMult = 1.10f;
    buy.MinBuyAmount = 1;

    // sell
    sell.SellNamedItem = false;
    sell.HubSellOrdersPerRefresh = 80;
    sell.SprinkleSellOrdersPerRefresh = 10;
    sell.SellItemMetaLevelMin = 0;
    sell.SellItemMetaLevelMax = 4;
    sell.MinSellAmount = 1;
}

bool MarketBotConf::ProcessBotConf(const TiXmlElement* ele)
{
    // entering element, extend allowed syntax
    AddMemberParser( "main",      &MarketBotConf::ProcessMain );
    AddMemberParser( "hubs",      &MarketBotConf::ProcessHubs );
    AddMemberParser( "buy",       &MarketBotConf::ProcessBuy );
    AddMemberParser( "sell",      &MarketBotConf::ProcessSell );

    // parse the element
    const bool result = ParseElementChildren( ele );

    // leaving element, reduce allowed syntax
    RemoveParser( "main" );
    RemoveParser( "hubs" );
    RemoveParser( "buy" );
    RemoveParser( "sell" );

    // return status of parsing
    return result;
}

bool MarketBotConf::ProcessMain(const TiXmlElement* ele)
{
    AddValueParser( "DataRefreshTime",          main.DataRefreshTime );
    AddValueParser( "OrderLifetime",            main.OrderLifetime );
    AddValueParser( "MaxISKPerOrder",           main.MaxISKPerOrder );

    const bool result = ParseElementChildren( ele );

    RemoveParser( "DataRefreshTime" );
    RemoveParser( "OrderLifetime" );
    RemoveParser( "MaxISKPerOrder" );

    return result;
}

bool MarketBotConf::ProcessHubs(const TiXmlElement* ele)
{
    hubs.stationIDs.clear();
    for (const TiXmlElement* child = ele->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (std::strcmp(child->Value(), "station") != 0)
            continue;
        uint32 sid = 0;
        const char* aid = child->Attribute("id");
        if (aid != nullptr)
            sid = static_cast<uint32>(std::strtoul(aid, nullptr, 10));
        else if (child->GetText() != nullptr)
            sid = static_cast<uint32>(std::strtoul(child->GetText(), nullptr, 10));
        if (sid != 0)
            hubs.stationIDs.push_back(sid);
    }
    return true;
}

bool MarketBotConf::ProcessBuy(const TiXmlElement* ele)
{
    AddValueParser( "HubBuyOrdersPerRefresh",       buy.HubBuyOrdersPerRefresh );
    AddValueParser( "SprinkleSystemsCount",         buy.SprinkleSystemsCount );
    AddValueParser( "SprinkleBuyOrdersPerRefresh",  buy.SprinkleBuyOrdersPerRefresh );
    AddValueParser( "HubBuyPriceMinMult",           buy.HubBuyPriceMinMult );
    AddValueParser( "HubBuyPriceMaxMult",           buy.HubBuyPriceMaxMult );
    AddValueParser( "SprinkleBuyPriceMinMult",      buy.SprinkleBuyPriceMinMult );
    AddValueParser( "SprinkleBuyPriceMaxMult",      buy.SprinkleBuyPriceMaxMult );
    AddValueParser( "MinBuyAmount",                 buy.MinBuyAmount );

    const bool result = ParseElementChildren( ele );

    RemoveParser( "HubBuyOrdersPerRefresh" );
    RemoveParser( "SprinkleSystemsCount" );
    RemoveParser( "SprinkleBuyOrdersPerRefresh" );
    RemoveParser( "HubBuyPriceMinMult" );
    RemoveParser( "HubBuyPriceMaxMult" );
    RemoveParser( "SprinkleBuyPriceMinMult" );
    RemoveParser( "SprinkleBuyPriceMaxMult" );
    RemoveParser( "MinBuyAmount" );

    return result;
}

bool MarketBotConf::ProcessSell(const TiXmlElement* ele)
{
    AddValueParser( "SellNamedItem",                sell.SellNamedItem );
    AddValueParser( "HubSellOrdersPerRefresh",      sell.HubSellOrdersPerRefresh );
    AddValueParser( "SprinkleSellOrdersPerRefresh", sell.SprinkleSellOrdersPerRefresh );
    AddValueParser( "MinSellAmount",                sell.MinSellAmount );
    AddValueParser( "SellItemMetaLevelMin",         sell.SellItemMetaLevelMin );
    AddValueParser( "SellItemMetaLevelMax",         sell.SellItemMetaLevelMax );

    const bool result = ParseElementChildren( ele );

    RemoveParser( "SellNamedItem" );
    RemoveParser( "HubSellOrdersPerRefresh" );
    RemoveParser( "SprinkleSellOrdersPerRefresh" );
    RemoveParser( "MinSellAmount" );
    RemoveParser( "SellItemMetaLevelMax" );
    RemoveParser( "SellItemMetaLevelMin" );

    return result;
}

