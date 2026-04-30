# Destiny code vs public Tranquility netcode narrative

How EVEmu’s Destiny and entity loop relate to common **public** descriptions of EVE Online’s Tranquility stack (server authority, “~1 Hz” ticks, TiDi, NATS, Stackless Python). This is for operator/developer orientation, not a claim of internal CCP parity.

## What public summaries usually claim

- **Server authority:** authoritative simulation on the server; clients receive updates.
- **Tick / cadence:** often **~1 Hz** simulation for large-scale combat, with **TiDi** slowing time under extreme load.
- **Infra:** **Stackless Python**, **NATS** (or similar) for high message volume, separation of concerns.
- **Latency:** coarse ticks mean inputs can feel delayed; design tolerates high RTT.

## What EVEmu does

### Two timers (not one global simulation Hz)

[`EntityList::Process`](../../src/eve-server/EntityList.cpp):

1. **`m_stampTimer` (1000 ms)** — increments `GetStamp()`, runs **`SystemManager::ProcessTic()`** per second → **`SystemEntity::Process()` → `DestinyManager::Process()`** (1 Hz destiny path for the pieces wired there).
2. **`m_dynamicDestinyTimer` (`DynamicDestinyMs`, default 17 ms, clamped 10–100)** — **`SystemManager::ProcessDynamicDestiny()`** → **`DestinyManager::ProcessHighFreqState()`** on entities.

[`SystemManager`](../../src/eve-server/system/SystemManager.cpp): `ProcessTic()` is the 1 Hz system tic; `ProcessDynamicDestiny()` only runs high-freq destiny.

### Destiny: authoritative, split by phase

[`DestinyManager::Process` / `ProcessHighFreqState`](../../src/eve-server/system/DestinyManager.cpp):

- **1 Hz (`Process`):** pilot **in-warp tunnel** (`WARP` + `m_warpState`), **missiles**, **cloak proximity** throttled on `GetStamp()`. Warp cruise uses second-based delta from **`GetStamp() - m_stateStamp`**.
- **High freq (`ProcessHighFreqState` → `ProcessState`):** **subwarp** STOP/GOTO/ORBIT/FOLLOW and **warp align** (pre-tunnel). **`MoveObject()`** integrates using wall-clock **`posDt`** (bounded) with nominal step from **`DynamicDestinyMs`**.

So the public “everything is ~1 Hz” story matches EVEmu **most closely for warp tunnel and missile-style logic**, **not** for general subwarp motion.

### Net / observers

Piloted ships avoid sending every high-freq correction to the **owning** client (prediction / rubber-band mitigation). **Observers** get position updates throttled by **`destinyObserverBroadcastMinMs`** in [`eve-server.xml`](../../utils/config/eve-server.xml) (default 33 ms), unless **`tranquilityStyleObserverNet`** is true (then at most **once per `GetStamp()`**, ~1 Hz). See [`DestinyManager::MoveObject`](../../src/eve-server/system/DestinyManager.cpp).

## Remote hosting checklist

- **Game and image TCP:** [`Socket::set_tcp_nodelay`](../../src/eve-core/network/Socket.cpp) on the main server accept path; image server sets `tcp::no_delay` on accept ([`ImageServerListener::HandleAccept`](../../src/eve-server/imageserver/ImageServerListener.cpp)). Reduces small-packet batching over WAN.
- **`ServerSleepTime`:** Lower (e.g. 5 ms) can tighten the main loop on a dedicated host; costs CPU.
- **`DynamicDestinyMs`:** Default 17 ms is a good balance; raising it lowers sim rate for NPC/subwarp integration.
- **`destinyObserverBroadcastMinMs`:** Lower (e.g. 20) can smooth **other players** seeing your ship remotely; raises bandwidth. **Ignored** when `tranquilityStyleObserverNet` is true.
- **`tranquilityStyleObserverNet`:** Set `true` to approximate coarse **observer** updates (~1 Hz) while server still integrates subwarp at `DynamicDestinyMs`. Expect steppier motion for spectators.
- **Diagnostics:** Enable destiny log categories (e.g. `DESTINY__MOVE_DEBUG`) in `log.ini` when tuning; watch CPU and client rubber-banding.
- **VPN / MTU:** Hamachi and similar add RTT; path MTU issues can manifest as stalls unrelated to destiny tuning.

## Comparison table

| Aspect | Tranquility (public narrative) | EVEmu |
|--------|-------------------------------|--------|
| Runtime | Stackless Python | C++ |
| Messaging | NATS-scale bus | In-process + game TCP |
| Overload | TiDi | None (fixed timers; host capacity) |
| Sim cadence | Often described ~1 Hz globally | **Dual:** 1 Hz stamp + **~10–100 Hz** subwarp (`DynamicDestinyMs`) |

## Verdict

- **Aligned:** Server-authoritative Destiny; **`GetStamp`** in protocol/state; 1 Hz driving much **system/entity** work and **warp tunnel** phases.
- **Not aligned:** Subwarp deliberately **faster than 1 Hz** (comments note prior 1 Hz–only subwarp hurt observers vs `GetAuthPosition`). No TiDi, NATS, or Stackless stack in this codebase.

## Optional future work (product choice)

Driving **subwarp** purely off **stamp-quantized** steps would move closer to a strict “1 Hz sim” **story** but would reopen **extrapolation / observer** issues the current design avoids. Not required for Crucible client protocol compatibility; would be behavioral experimentation.
