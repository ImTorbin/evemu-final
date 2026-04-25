# Captain's Quarters room module — patch guide

The CQ room module is the singular client-side entry point that builds the
3D scene, mounts the local avatar, and tears down on undock / log-off.
Crucible builds vary the exact path, so Phase 1 (`12_grep_modules.ps1`)
gives you the ground-truth. The four candidate paths from research are:

```
eve/client/script/environment/captainsQuartersRoom.py
eve/client/script/ui/station/captainsQuartersRoom.py
eve/client/script/environment/cqRoom.py
eve/client/script/ui/station/captainsQuarters/captainsQuartersRoom.py
```

Whichever one your dump exposes, apply the four edits below. They are
written as unified-diff fragments anchored to the kind of construct you
should grep for; finalize them against the real line numbers when you have
the source.

---

## Edit 1 — register `SharedCQClient` in the boot service list

The Carbon `service.py` builds its list of boot services from a module
identified by `__servicemodules__` or `BOOT_SERVICES` (depending on which
client.dll variant you're on). Find that module in the decompiled tree:

```powershell
Get-ChildItem -Path eve_code -Recurse -Filter *.py |
    Select-String -Pattern 'BOOT_SERVICES|__servicemodules__|RegisterService' -List |
    Select-Object Path, LineNumber, Line
```

Add `'sharedCQ'` to the boot list near the other singleton clients
(`'sceneManager'`, `'paperDoll'`, `'bracket'`):

```diff
 BOOT_SERVICES = [
     'machoNet',
     'paperDoll',
     'sceneManager',
+    'sharedCQ',
     ...
 ]
```

If the client uses lazy registration via `service.GetService`, you can
instead wire the import into `eve/client/script/environment/__init__.py`
(or its equivalent) so `SharedCQClient` is forced into the import graph
and `sm.GetService('sharedCQ')` lazy-instantiates it on first use.

---

## Edit 2 — call `OnSceneReady` once the local CQ scene is built

Grep the CQ room module for the post-construction callback. In Crucible
research it shows up as one of:

- `def OnSceneLoaded(self, ...):`
- `def _BuildScene(self, ...):` (right before `return self.scene`)
- `def EnterCQ(self, ...):` (final line)

Anchor the patch right after the local avatar has been added to the scene
and the camera has been wired up:

```diff
     def OnSceneLoaded(self, scene):
         ...
         self.scene = scene
         self.localAvatar = self._BuildLocalAvatar(scene)
+        try:
+            sm.GetService('sharedCQ').OnSceneReady(scene, session.stationid)
+        except Exception as e:
+            log.LogException('captainsQuartersRoom: sharedCQ OnSceneReady failed: %s' % e)
         return scene
```

If the room exposes a richer "scene + avatar" object instead of a bare
scene (some Crucible variants do), pass that object — the service is
liberal about what `scene.AddEntity` looks like (see `_Spawn` fallbacks in
`sharedCQ.py`).

---

## Edit 3 — pump position updates from the local avatar tick

The CQ room runs a per-frame avatar update somewhere in
`Update`/`OnUpdate`/`OnAvatarMove`. Locate the spot where the local
avatar's `(x, y, z)` is computed (or read it back from the avatar object),
then forward to the service. The service rate-limits internally:

```diff
     def OnAvatarMove(self):
         pos = self.localAvatar.GetWorldPosition()
+        try:
+            sm.GetService('sharedCQ').UpdateLocalTransform(pos[0], pos[1], pos[2])
+        except Exception as e:
+            log.LogException('captainsQuartersRoom: sharedCQ UpdateLocalTransform failed: %s' % e)
```

If the room module doesn't expose a granular avatar-move tick, pump the
update from the room's per-frame `Update` method instead — the rate
limiter will flatten it back to ~4 Hz.

---

## Edit 4 — tear down on leave / undock / log-off

Find the room's teardown path. Typical names: `Stop`, `Unload`, `LeaveCQ`,
`OnExitCQ`, `Destroy`. Add the service call before the room releases the
scene reference so we still have a valid `self.scene` on `SharedCQClient`:

```diff
     def Unload(self):
+        try:
+            sm.GetService('sharedCQ').Stop()
+        except Exception as e:
+            log.LogException('captainsQuartersRoom: sharedCQ.Stop failed: %s' % e)
         if self.scene is not None:
             self.scene.Unload()
             self.scene = None
```

`SharedCQClient.Stop()` calls `LeaveSharedQuarters` on the bound CQ object,
which the EVEmu server uses to remove this character from the worldspace
and broadcast `OnCharNoLongerInStation` to other occupants.

---

## Edit 5 — audit `myAvatar` singletons

Search the CQ room module (and its imports) for hard-coded singular
references to the local avatar:

```powershell
Select-String -Path "<the CQ room module>" -Pattern 'myCharacter|myAvatar|selfAvatar|self\.avatar([^s]|$)'
```

For each hit, decide whether the assumption "exactly one avatar in the
scene" is structural (it usually is — interaction hotspots, camera
follow, the chair, the door triggers, the holographic pet) or only
cosmetic. The server only ever drives **one local avatar per client**, so
hotspots and camera follow can keep the singular reference. What you do
need to guard:

1. **ProximityTriggerService** — the service that threw the original
   `KeyError: 1`. It iterates the scene's entities to find what's near a
   trigger zone. Find its `_OnEnter` / `_OnExit` callbacks (Phase 1's
   `12_grep_modules.ps1` records the file). Wherever it accesses
   `entity.charID` or `entity.is_local_player`, add an early-return for
   remote avatars:

   ```diff
        def _OnEnter(self, entity, triggerID):
   +        if getattr(entity, 'isRemoteCQAvatar', False):
   +            return
            ...
   ```

   Then in `sharedCQ.py._Spawn`, set the marker right after construction:

   ```diff
            avatar = paperDoll.PaperDollAvatar(...)
   +        try: avatar.isRemoteCQAvatar = True
   +        except Exception: pass
            ...
   ```

2. **BracketClient** — bracket overlays may try to draw a status icon
   over each avatar. Vanilla CQ never had to deal with non-local avatars
   in this scene, so it may crash on missing per-avatar data. Add the
   same `isRemoteCQAvatar` early-return at the bracket creation callback
   if the grep shows it touching avatars by entity reference.

3. **`scene.IterateEntities()` / `scene.dynamics`** consumers in the room
   module — anywhere the room counts or filters entities by avatar,
   verify the counter is local-only or rewrite it to filter by
   `isRemoteCQAvatar`.

---

## Verification

After applying the five edits and copying `sharedCQ.py` into the dumped
tree, run:

```powershell
Select-String -Path eve_code -Recurse -Pattern "sharedCQ" -Include *.py
```

You should see:
- the service module itself (`sharedCQ.py`)
- the boot list edit (Edit 1)
- the four call sites in the CQ room module (Edits 2-4 and the proximity-
  trigger guard)

If those show up, you're ready for `40_rebuild_client.ps1`.
