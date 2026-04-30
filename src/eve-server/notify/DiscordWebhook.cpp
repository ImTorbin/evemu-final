/*
    ------------------------------------------------------------------------------------
    Discord webhook delivery (optional libcurl). Game thread enqueues; worker POSTs.
    ------------------------------------------------------------------------------------
*/

#include "eve-server.h"

#include "EVEServerConfig.h"
#include "notify/DiscordWebhook.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif

namespace {

#ifdef HAVE_LIBCURL
struct PostJob {
    std::string url;
    std::string body;
};

std::mutex                           s_queueMutex;
std::condition_variable              s_queueCv;
std::queue<PostJob>                 s_queue;
std::thread                         s_workerThread;
std::atomic_bool                    s_started{ false };
std::atomic_bool                    s_stop{ false };

/** True after CurlEnsureGlobal() runs (sync server-up POST or async worker startup). */
std::atomic_bool                    s_curlGlobalReady{ false };

std::mutex                              s_cooldownMutex;
std::chrono::steady_clock::time_point  s_lastRareLootPost{};
bool                                    s_rareLootCooldownInit{ false };

std::chrono::steady_clock::time_point  s_lastDeathPost{};
bool                                    s_deathCooldownInit{ false };

size_t CurlDiscardWrite(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    (void)ptr;
    (void)userdata;
    return size * nmemb;
}

void CurlEnsureGlobal()
{
    static std::once_flag initOnce;
    std::call_once(initOnce, []() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        s_curlGlobalReady.store(true);
    });
}

void WorkerLoop()
{
    for (;;) {
        PostJob job;
        {
            std::unique_lock lk(s_queueMutex);
            s_queueCv.wait(lk, [] { return s_stop.load() || !s_queue.empty(); });
            if (s_stop && s_queue.empty())
                return;
            if (s_queue.empty())
                continue;
            job = std::move(s_queue.front());
            s_queue.pop();
        }

        CURL *curl = curl_easy_init();
        if (curl == nullptr) {
            sLog.Error("DiscordWebhook", "curl_easy_init failed; queued webhook dropped.");
            continue;
        }

        struct curl_slist *hdrs = nullptr;
        hdrs = curl_slist_append(hdrs, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, job.url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, job.body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(job.body.size()));
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlDiscardWrite);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            sLog.Error("DiscordWebhook", "curl_easy_perform failed: %s", curl_easy_strerror(res));

        curl_slist_free_all(hdrs);
        curl_easy_cleanup(curl);
    }
}

void EnsureWorkerStarted()
{
    CurlEnsureGlobal();
    bool expected = false;
    if (!s_started.compare_exchange_strong(expected, true))
        return;
    s_workerThread = std::thread(WorkerLoop);
}

std::string JsonEscape(const std::string& in)
{
    std::ostringstream o;
    for (unsigned char uc : in) {
        switch (uc) {
            case '\"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                o << static_cast<char>(uc);
        }
    }
    return o.str();
}

constexpr int COLOR_LOOT = 0x2ecc71;
constexpr int COLOR_DEATH = 0xe74c3c;
/** Deep space blue — closer to EVE UI than flat bootstrap blue. */
constexpr int COLOR_UP = 0x1c2833;

/** Static type renders via images.evetech.net (EVE static data artwork; not affiliated with CCP). */
constexpr const char* const SERVER_UP_THUMB_URL = "https://images.evetech.net/types/1230/render?size=128";
constexpr const char* const SERVER_UP_IMAGE_URL = "https://images.evetech.net/types/11021/render?size=512";

void QueuePost(const std::string& url, const std::string& jsonBody)
{
    if (url.empty())
        return;
    EnsureWorkerStarted();
    {
        std::lock_guard lk(s_queueMutex);
        s_queue.push(PostJob{ url, jsonBody });
    }
    s_queueCv.notify_one();
}

bool RareLootCooldownAllows(uint32 cooldownSeconds)
{
    if (cooldownSeconds == 0)
        return true;
    auto now = std::chrono::steady_clock::now();
    std::lock_guard lk(s_cooldownMutex);
    if (s_rareLootCooldownInit) {
        const auto delta = std::chrono::duration_cast<std::chrono::seconds>(now - s_lastRareLootPost).count();
        if (delta < static_cast<long long>(cooldownSeconds))
            return false;
    }
    s_lastRareLootPost = now;
    s_rareLootCooldownInit = true;
    return true;
}

bool ExpensiveDeathCooldownAllows(uint32 cooldownSeconds)
{
    if (cooldownSeconds == 0)
        return true;
    auto now = std::chrono::steady_clock::now();
    std::lock_guard lk(s_cooldownMutex);
    if (s_deathCooldownInit) {
        const auto delta = std::chrono::duration_cast<std::chrono::seconds>(now - s_lastDeathPost).count();
        if (delta < static_cast<long long>(cooldownSeconds))
            return false;
    }
    s_lastDeathPost = now;
    s_deathCooldownInit = true;
    return true;
}
#endif /* HAVE_LIBCURL */

} // namespace

void DiscordWebhook_Shutdown()
{
#ifdef HAVE_LIBCURL
    if (s_started.load()) {
        {
            std::lock_guard lk(s_queueMutex);
            s_stop = true;
        }
        s_queueCv.notify_one();
        if (s_workerThread.joinable())
            s_workerThread.join();
    }
    if (s_curlGlobalReady.exchange(false))
        curl_global_cleanup();
#endif
}

void DiscordWebhook_NotifyRareLoot(
    const std::string& wreckSourceName,
    uint32 groupID,
    uint32 solarSystemID,
    const std::string& solarSystemName,
    const std::vector<DiscordLootLine>& lootLines,
    bool killerIsPlayerCharacter)
{
#ifndef HAVE_LIBCURL
    (void)wreckSourceName;
    (void)groupID;
    (void)solarSystemID;
    (void)solarSystemName;
    (void)lootLines;
    (void)killerIsPlayerCharacter;
#else
    const std::string& url = sConfig.discord.webhookRareLootURL;
    if (url.empty())
        return;
    /* Same idea as server-up: webhook URL present means post; do not require <discord><enabled>true</enabled>. */
    if (sConfig.discord.lootOnlyPlayerShipKills && !killerIsPlayerCharacter)
        return;
    if (!RareLootCooldownAllows(sConfig.discord.cooldownRareLootSeconds))
        return;

    const unsigned floor = std::min(255u, static_cast<unsigned>(sConfig.discord.rareLootMetaGt));
    std::vector<DiscordLootLine> rare;
    rare.reserve(lootLines.size());
    double totalRareISK = 0.0;
    for (const auto& L : lootLines) {
        if (static_cast<unsigned>(L.metaLvl) > floor) {
            rare.push_back(L);
            totalRareISK += L.lineISK;
        }
    }
    if (rare.empty())
        return;

    std::sort(rare.begin(), rare.end(),
              [](const DiscordLootLine& a, const DiscordLootLine& b) {
                  if (a.metaLvl != b.metaLvl)
                      return a.metaLvl > b.metaLvl;
                  return a.lineISK > b.lineISK;
              });

    std::ostringstream desc;
    desc << "High-meta loot (**meta level > " << floor << "**). ";
    desc.setf(std::ios::fixed);
    desc.precision(0);
    desc << "**~ " << totalRareISK << "** ISK in matching lines (Σ basePrice × qty for those rolls)\n";
    desc.unsetf(std::ios::floatfield);
    desc << "**System:** " << solarSystemName << " (" << solarSystemID << "), **group:** " << groupID;
    desc << ", **killer is PC:** " << (killerIsPlayerCharacter ? "yes" : "no");

    constexpr size_t kMaxLinesInEmbed = 8;
    desc << "\n\n**High-meta rolls:**";
    size_t shown = 0;
    for (const auto& L : rare) {
        if (shown >= kMaxLinesInEmbed)
            break;
        std::ostringstream line;
        line << "\n×" << L.qty << " type " << L.typeID << ", **meta " << static_cast<unsigned>(L.metaLvl) << "**, ~";
        line.setf(std::ios::fixed);
        line.precision(0);
        line << L.lineISK;
        line.unsetf(std::ios::floatfield);
        line << " ISK";
        desc << line.str();
        ++shown;
    }

    std::ostringstream json;
    json << "{\"embeds\":[{\"title\":\"" << JsonEscape("Rare wreck loot: " + wreckSourceName) << "\",";
    json << "\"description\":\"" << JsonEscape(desc.str()) << "\",\"color\":" << COLOR_LOOT << "}]}";

    QueuePost(url, json.str());
#endif
}

void DiscordWebhook_NotifyExpensiveDeath(
    const std::string& victimName,
    uint32 victimCharID,
    const std::string& shipTypeName,
    uint32 solarSystemID,
    const std::string& solarSystemName,
    double hullISK,
    double cargoISK,
    const std::string& killerDisplayName,
    uint32 killerCharID)
{
#ifndef HAVE_LIBCURL
    (void)victimName;
    (void)victimCharID;
    (void)shipTypeName;
    (void)solarSystemID;
    (void)solarSystemName;
    (void)hullISK;
    (void)cargoISK;
    (void)killerDisplayName;
    (void)killerCharID;
#else
    if (!sConfig.discord.enabled)
        return;
    const std::string& url = sConfig.discord.webhookExpensiveDeathURL;
    if (url.empty())
        return;

    double totalISK = hullISK + cargoISK;
    if (totalISK + 1.0 < sConfig.discord.minExpensiveDeathEstimatedISK)
        return;
    if (!ExpensiveDeathCooldownAllows(sConfig.discord.cooldownExpensiveDeathSeconds))
        return;

    std::ostringstream desc;
    desc << "**Victim:** " << victimName << " (" << victimCharID << ")\n";
    desc << "**Ship:** " << shipTypeName << "\n";
    desc.setf(std::ios::fixed);
    desc.precision(0);
    desc << "**Estimated loss:** hull " << hullISK << ", cargo+fittings " << cargoISK << ", Σ " << totalISK << "\n";
    desc.unsetf(std::ios::floatfield);
    desc << "**Where:** " << solarSystemName << " (" << solarSystemID << ")\n";
    desc << "**Killer:** " << killerDisplayName;
    if (killerCharID != 0u)
        desc << " (" << killerCharID << ")";

    std::ostringstream json;
    json << "{\"embeds\":[{\"title\":\"" << JsonEscape("Ship loss") << "\",";
    json << "\"description\":\"" << JsonEscape(desc.str()) << "\",\"color\":" << COLOR_DEATH << "}]}";

    QueuePost(url, json.str());
#endif
}

void DiscordWebhook_NotifyServerUp(const std::string& startedLocalTime, uint32 playersOnline, uint32 maxPlayersCap)
{
#ifndef HAVE_LIBCURL
    (void)startedLocalTime;
    (void)playersOnline;
    (void)maxPlayersCap;
#else
    /* Server-up is infra signal: do not gate on discord.enabled (operators often disable loot/death
     * spam but still want restart notifications). Rare loot/death hooks still honor enabled + URLs. */
    const std::string& url = sConfig.discord.webhookServerUpURL;
    if (url.empty()) {
        sLog.Yellow("DiscordWebhook",
            "Server-up webhook skipped: <webhookServerUpURL> is empty in eve-server.xml.");
        return;
    }

    std::ostringstream descUp;
    descUp << "The cluster is **accepting connections**. Jump in when you are ready.\n\n";
    descUp << "**Capsuleers online:** " << playersOnline;
    descUp << "\n**Capacity:** up to " << maxPlayersCap << " pilots";
    descUp << "\n**Local time (server host):** " << startedLocalTime;

    std::ostringstream jsonUp;
    jsonUp << "{\"embeds\":[{\"title\":\"" << JsonEscape("Capsuleers welcome — New Eden is open") << "\",";
    jsonUp << "\"description\":\"" << JsonEscape(descUp.str()) << "\",";
    jsonUp << "\"color\":" << COLOR_UP << ",";
    jsonUp << "\"thumbnail\":{\"url\":\"" << JsonEscape(SERVER_UP_THUMB_URL) << "\"},";
    jsonUp << "\"image\":{\"url\":\"" << JsonEscape(SERVER_UP_IMAGE_URL) << "\"},";
    jsonUp << "\"footer\":{\"text\":\"" << JsonEscape("EVEmu | Ship renders: evetech.net") << "\"}";
    jsonUp << "}]}";

    const std::string body = jsonUp.str();

    /* Synchronous POST so restart always yields an immediate success/failure line and is not dropped
     * if curl_easy_init fails in the async worker path (previously silent). */
    CurlEnsureGlobal();
    CURL *curl = curl_easy_init();
    if (curl == nullptr) {
        sLog.Error("DiscordWebhook", "Server-up webhook failed: curl_easy_init returned null.");
        return;
    }

    struct curl_slist *hdrs = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlDiscardWrite);

    const CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        sLog.Error("DiscordWebhook", "Server-up webhook HTTPS POST failed: %s", curl_easy_strerror(res));
    else if (httpCode < 200 || httpCode >= 300)
        sLog.Error("DiscordWebhook", "Server-up webhook rejected by Discord (HTTP %li). Check webhook URL/token.", httpCode);
    else
        sLog.Green("DiscordWebhook", "Server-up notification sent to Discord.");
#endif
}