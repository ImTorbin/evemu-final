# Parallel `ProcessDynamicDestiny` (design notes)

Today [`EntityList::Process`](EntityList.cpp) runs [`SystemManager::ProcessDynamicDestiny`](SystemManager.cpp) **sequentially** for every loaded system. Each [`ProcessDynamicDestiny`](SystemManager.cpp) walks `m_ticEntities` and calls [`DestinyManager::ProcessHighFreqState`](DestinyManager.cpp); iteration can restart when `m_entityChanged` is set (entity add/remove during processing).

## Why parallelization is non-trivial

1. **Map mutation** — `itr->second->ProcessHighFreqState()` may add/remove/move entities, setting `m_entityChanged` and forcing `itr = m_ticEntities.begin()`. Two threads iterating the same map would race.
2. **Cross-system coupling** — Mostly absent for destiny integration; each `SystemEntity` destiny touches its bubble and clients. Risk is low for **system-isolated** work if each system’s `SystemManager` is owned by one logical unit of work.
3. **Safe unit of work** — A **system** is the natural shard: process all `ticEntities` in system A on worker N without touching system B’s maps.

## Candidate approach (future implementation)

1. Collect `SystemManager*` pointers from `sEntityList` for systems that are loaded.
2. Use a thread pool (worker count ≤ hardware, e.g. cap at 16–32 to limit contention).
3. Each task runs **only** `ProcessDynamicDestiny()` for **one** system, on that system’s `m_ticEntities`.
4. **Barrier** after all tasks complete before the next `m_dynamicDestinyTimer` tick.
5. If `ProcessDynamicDestiny` still mutates global state beyond that `SystemManager`, audit those call paths (unlikely for pure `MoveObject`).

## Preconditions before coding

- Snapshot **per-system** invariant: no destiny call from system A resolves a `SystemEntity` in system B in a way that grabs another system’s mutex incorrectly.
- Confirm `m_entityChanged` restart remains **single-threaded per system** (no iterator sharing).
- Stress-test: many NPCs in one system still single-threaded for that system; parallel only helps **many systems** loaded at once.

## When not worth it

- Single busy grid (one system dominates) → parallel across systems does not reduce that hotspot; only **per-entity** parallelism or cheaper `MoveObject` helps.
- Lock overhead may exceed gains for small shard counts.
