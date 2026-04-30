# Beyonce bound object → DestinyManager reference

Client `michelle` remote park calls map to [`BeyonceService.cpp`](BeyonceService.cpp) / [`BeyonceBound`](BeyonceService.h). Primary destiny packets are defined in [`eve-common/packets/Destiny.xmlp`](../../eve-common/packets/Destiny.xmlp) and encoded via [`packets/Destiny.h`](../../eve-common/packets/Destiny.h).

Shared prechecks in many handlers: `DestinyMgr()` non-null, `IsWarping()`, `AbortIfLoginWarping()`, `IsFrozen()`. Fleet warps use `CanIssueWarp()` (also checks frozen / login warp / warp scramble attr).

| Bound RPC | DestinyManager API | Notes / emitted commands (typical) |
|-----------|---------------------|-----------------------------------|
| `CmdFollowBall` | `Follow(pSE, distance)` | `CmdFollowBall`; integrates via `ProcessHighFreqState` → `Follow()` → `MoveObject`. |
| `CmdSetSpeedFraction` | `SetSpeedFraction(fraction)` or `SetSpeedFraction(fraction, true)` | `CmdSetSpeedFraction` when changing curve. |
| `CmdAlignTo` | `AlignTo(pEntity)` | Wrapper for `Follow(pSE, 0)`. |
| `CmdGotoDirection` | `GotoDirection(dir)` | `CmdGotoDirection`; `Ball::GOTO` to distant point on ray. |
| `CmdGotoBookmark` | `GotoPoint(point)` | `CmdGotoPoint` from bookmark coords or celestial position. |
| `CmdOrbit` | `Orbit(pEntity, range)` | `CmdOrbit` + orbit setup; tick integration in protected `Orbit()`. |
| `CmdWarpToStuff` | `WarpTo(warpToPoint, distance)` or `DoFleetWarp` | `CmdWarpTo` + FX; fleet branch warps subordinates via `WarpTo` per ship. |
| `CmdWarpToStuffAutopilot` | `WarpTo(pos, distance, true, pSE)` | Autopilot flag; follow semantics after warp. |
| `CmdStop` | `Stop()` | Halt / `CmdStop` path in destiny. |
| `CmdDock` | `AttemptDockOperation()` | Dock radius / approach; not purely translation. |
| `CmdStargateJump` | *(none direct)* | `Client::StargateJump`; session change — see `Client.cpp`. |
| `UpdateStateRequest` | `SendSetState()` | Full ballpark refresh. |
| `CmdAbandonLoot` | — | Wreck `Abandon()` + slim items; no ship destiny. |
| `CmdFleetRegroup` | `DoFleetWarp(issuer, pos, 2500, false, true)` | Regroup: subordinates `WarpTo` only (issuer stays). |
| `CmdFleetTagTarget` | — | Stub / logging. |
| `CmdBeaconJumpFleet` / `Alliance` | *(cyno bridge)* | `Client::CynoJump(beacon)` after validation. |
| `CmdJumpThroughFleet` / `Alliance` / `CorpStructure` | `DestinyMgr()` gates only | Jump portal → `CynoJump` where applicable. |

`AlignTo` → [`DestinyManager::Follow`](../system/DestinyManager.cpp) at range 0.

Fleet warp helper `DoFleetWarp`: eligible fleet members get `WarpTo(where, distance)` on their ship destiny (same as issuer destination math).

For internal packet names when tracing, search `DestinyManager.cpp` for `CmdWarpTo`, `CmdGotoPoint`, `CmdFollowBall`, `CmdOrbit`, `SetBallPosition`, `SetBallVelocity`, `SendDestinyUpdate`.

## Profiling (MoveObject / Orbit / SendDestinyUpdate)

With `<UseProfiling>true</UseProfiling>` in [`utils/config/eve-server.xml`](../../../utils/config/eve-server.xml), server `PrintProfile` includes:

- **DestMoveObj** — wall time per `MoveObject()` (high-frequency integration).
- **DestOrbitTic** — wall time per protected `Orbit()` tick.
- **DestSendUpd** — wall time per `SendDestinyUpdate` (bubble fan-out or pilot queue).

Counts scale with `DynamicDestinyMs` and entity count; use for single-bubble NPC + multi-client stress.

