# Two-client smoke test

Goal: prove that two clients in the same station see each other's avatars
in Captain's Quarters.

## Prep

- EVEmu server built with the C++ changes from this branch
  (`CQManager.cpp`, `PaperDollService.{h,cpp}`).
- `locationScenes` seeded for the dev station (`sql/locationScenes_helper.sql`).
- Two distinct EVE accounts, each with one Crucible character that lives
  in the same station.
- Two Crucible installs (or one install side-loaded twice via
  `eveloader2`), both with the rebuilt `compiled.code` and patched
  `blue.dll` from `40_rebuild_client.ps1`.
- `EnableSharedCaptainsQuarters` enabled in `utils/config/eve-server.xml`.

## Server log

Tail the server stdout (or `logs/server.log`). Open both client
sessions sequentially and dock both characters in the same station.
Expected lines, in order:

1. First character logs in / docks:
   ```
   [CQ] EnsureWorldSpaceForStation: created in-memory worldSpace <ws> for station <st>
   [CQ] Join: char=<A> ... worldSpace=<ws> occupantsNow=1 (broadcast spawn to others)
   ```
2. Second character logs in / docks:
   ```
   [CQ] EnsureWorldSpaceForStation: reuse worldSpace <ws> for station <st> (occupants=1)
   [CQ] Join: char=<B> ... worldSpace=<ws> occupantsNow=2 (broadcast spawn to others)
   [CQ] Join: sent OnCharNowInStation fallback to char=<A> for joinedChar=<B>
   [CQ] Join: sent reciprocal OnCharNowInStation to joinedChar=<B> about existingChar=<A>
   [CQ] Join: replay OnCQAvatarMove to joinedChar=<B> about existingChar=<A>
   [CQ] BroadcastTransform: OnCQAvatarMove -> char=<A> ... fromChar=<B> ...
   ```
3. Walk B's avatar around — every ~250ms you should see:
   ```
   [CQ] UpdatePosition: char=<B> ... pos=(...)
   [CQ] BroadcastTransform: OnCQAvatarMove -> char=<A> ... fromChar=<B> ...
   ```

   Critically, the `OnMultiEvent(OnCQAvatarMove A/B)` lines should NOT
   appear anymore — those were removed in Phase 2c.

4. Either character logs out or undocks:
   ```
   [CQ] Leave: char=<X> ... worldSpace=<ws> occupantsNow=...
   ```

## Client log

Both clients write logs under `<EVEInstall>\cache\logs\Client\` (note the
launcher option `/LUA:OFF` enables more verbose output). Tail both. On
client A, after client B docks, expect:

```
[svc.sharedCQClient] OnCQAvatarMove char=<B> ws=<ws> pos=(...) bloodline=... race=...
[svc.sharedCQClient] spawned remote avatar char=<B> race=... bloodline=... gender=... pos=...
```

When B moves:

```
[svc.sharedCQClient] OnCQAvatarMove char=<B> ws=<ws> pos=(...)
```

When B leaves:

```
[svc.sharedCQClient] OnCharNoLongerInStation char=<B>
[svc.sharedCQClient] despawned remote avatar char=<B>
```

## Visual

On client A, expect a second avatar to appear in the CQ scene at the
position B is at. Common partial-success states:

- Avatar appears but is in T-pose: Morpheme animation graph not loaded
  for that bloodline. The four bloodline graphs ship by default; if you
  see this, check that `resPaperDollC.stuff` / `resPaperDollG.stuff` /
  `resPaperDollM.stuff` / `resPaperDollA.stuff` are all present in
  `<EVEInstall>\res\`. T-pose is acceptable for the first verification
  pass — animation polish is a follow-up.
- Avatar appears but with default colors (white skin / black hair):
  `paperDollServer.GetPaperDollDataFor` returned an empty KeyVal because
  the second character has no rows in `avatar_colors` /
  `avatar_modifiers`. Run a character through the in-game customization
  flow once and the data will populate.
- Avatar does not appear, but server logs show the broadcast: open the
  client log for `[svc.sharedCQClient] PaperDollAvatar` exception lines.
  The constructor signature varies between Crucible builds; the service
  has a positional fallback but rare builds may need keyword tweaks.

## Failure → next step

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| Server shows `[CQ] BroadcastTransform: no online Client for charID X` for the second character | Second client hasn't actually bound `captainsQuartersSvc` (CQ scene didn't load) | Check `locationScenes` seed (`sql/locationScenes_helper.sql` step 2) |
| Client log: `_BindCQ: no solarsystemid in session` | Session is missing `solarsystemid2` while docked | Compare against `Client.cpp` `InitSession` — ensure `solarsystemid2` is being set |
| Client log: `JoinSharedQuarters: <error>` | Bound RPC failed | Server log will show why; usually a missing CQ row or a stale `staCQInstances` entry |
| Client crashes on PaperDollAvatar instantiation | Constructor signature mismatch | Patch `_Spawn` in `sharedCQ.py` to use the actual constructor recorded in Phase 1 |
| Avatars appear, then drift apart from where the other client thinks they are | Rate-limit dropping legitimate updates, or local position read uses a different frame of reference than the room | Reduce `_rateLimitMs` in `sharedCQ.py` from 250 → 100, and confirm the room's `GetWorldPosition()` returns scene-local coordinates |
