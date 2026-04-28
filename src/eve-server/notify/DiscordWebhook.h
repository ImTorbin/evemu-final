/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    ------------------------------------------------------------------------------------
*/

#ifndef __DISCORD_WEBHOOK_H_INCL__
#define __DISCORD_WEBHOOK_H_INCL__

#include <cstdint>
#include <string>
#include <vector>

/** One rolled loot line: estimated ISK (basePrice × qty) and static type meta level (invTypes). */
struct DiscordLootLine {
    uint32      typeID{};
    uint32      qty{};
    double      lineISK{};
    uint8       metaLvl{};
};

void DiscordWebhook_Shutdown();

void DiscordWebhook_NotifyRareLoot(
    const std::string& wreckSourceName,
    uint32 groupID,
    uint32 solarSystemID,
    const std::string& solarSystemName,
    const std::vector<DiscordLootLine>& lootLines,
    bool killerIsPlayerCharacter);

void DiscordWebhook_NotifyExpensiveDeath(
    const std::string& victimName,
    uint32 victimCharID,
    const std::string& shipTypeName,
    uint32 solarSystemID,
    const std::string& solarSystemName,
    double hullISK,
    double cargoISK,
    const std::string& killerDisplayName,
    uint32 killerCharID);

/** Fires once when the game server thread is about to enter the main loop (post-reboot). */
void DiscordWebhook_NotifyServerUp(const std::string& startedUtc, const std::string& revision);

#endif /* !__DISCORD_WEBHOOK_H_INCL__ */
