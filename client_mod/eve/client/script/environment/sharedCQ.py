# sharedCQ.py
# -----------------------------------------------------------------------------
# Captain's Quarters multiplayer client service.
#
# Drop this file into the dumped client tree at:
#     <CodeRoot>/eve/client/script/environment/sharedCQ.py
# then rebuild compiled.code with `evecc compilecode`.
#
# Crucible's CQ scene was shipped as single-player; this service makes the
# scene render remote PaperDollAvatar instances using the snapshot returned
# by EVEmu's captainsQuartersSvc.JoinSharedQuarters and the OnCQAvatarMove
# notify the EVEmu server emits whenever any occupant moves.
#
# Server-side contracts assumed (delivered by the matching EVEmu server
# patches in src/eve-server/system/CQManager.cpp and
# src/eve-server/character/PaperDollService.cpp):
#
#   captainsQuartersSvc bound on `solarsystem:<solarSystemID>`:
#       JoinSharedQuarters()        -> (worldSpaceID, [9-tuple, ...])
#       LeaveSharedQuarters()       -> None
#       GetSnapshot()               -> [9-tuple, ...]
#       UpdateTransform(x, y, z)    -> None
#
#   paperDollServer (RemoteSvc):
#       GetPaperDollDataFor(charID) -> util.KeyVal(colors=..., modifiers=...,
#                                                  appearance=..., sculpts=...)
#
#   notify OnCQAvatarMove with key "charid":
#       (charID, x, y, z, worldSpaceID, corpID, gender, bloodlineID, raceID)
#
#   notify OnCharNoLongerInStation with key "stationid":
#       legacy nested tuple (charID, ...) -- consult Character.xmlp for the
#       full shape. We just pull the leading characterID.
#
# Conservative defaults: every client-side construction call (PaperDollAvatar
# creation, scene mutation, paperdoll RPC) is wrapped so a single broken
# remote avatar can never tear down the whole CQ scene.

import service
import blue
import sys
import log

# These imports may not exist in older Crucible builds. Phase 1 of the plan
# (12_grep_modules.ps1) records the actual paths; if any of these fail at
# import time, comment the failing import and patch _Spawn / _UpdateTransform
# to use the real module accessor.
try:
    import paperDoll
except ImportError:
    paperDoll = None


_LOG_TAG = 'svc.sharedCQClient'


class SharedCQClient(service.Service):
    """Client mirror of EVEmu's CQ broadcast pipeline."""

    __guid__ = 'svc.sharedCQClient'
    __servicename__ = 'sharedCQ'
    __displayname__ = 'Shared Captain\'s Quarters'
    __exportedcalls__ = {}
    __notifyevents__ = [
        'OnCQAvatarMove',
        'OnCharNowInStation',
        'OnCharNoLongerInStation',
    ]
    __dependencies__ = []

    # ----- service lifecycle -------------------------------------------------

    def Run(self, memStream=None):
        log.LogInfo('[%s] Run' % _LOG_TAG)
        self.remotes = {}             # charID -> PaperDollAvatar
        self.bound = None             # captainsQuartersSvc bound object
        self.sceneRef = None          # weak-ish ref to the active CQ scene
        self.localCharID = None
        self.localStationID = None
        self.lastTransformSent = None
        self._rateLimitMs = 250       # cap UpdateTransform to ~4 Hz

    def Stop(self, memStream=None):
        log.LogInfo('[%s] Stop' % _LOG_TAG)
        for charID, avatar in list(self.remotes.iteritems()):
            self._SafeUnload(charID, avatar)
        self.remotes.clear()
        if self.bound is not None:
            try:
                self.bound.LeaveSharedQuarters()
            except Exception as e:
                log.LogException('[%s] LeaveSharedQuarters during Stop: %s' % (_LOG_TAG, e))
            self.bound = None
        self.sceneRef = None
        self.localCharID = None
        self.localStationID = None
        self.lastTransformSent = None

    # ----- public entry points called from the patched CQ room ---------------

    def OnSceneReady(self, scene, stationID):
        """Patched CQ-room module calls this once the local avatar's scene is
        constructed. We bind captainsQuartersSvc, ask for the current
        snapshot, and spawn a remote avatar for every other occupant."""
        log.LogInfo('[%s] OnSceneReady: station=%s scene=%r' % (_LOG_TAG, stationID, scene))
        self.sceneRef = scene
        self.localStationID = stationID
        try:
            self.localCharID = session.charid
        except Exception:
            self.localCharID = None

        try:
            self.bound = self._BindCQ(stationID)
        except Exception as e:
            log.LogException('[%s] _BindCQ failed: %s' % (_LOG_TAG, e))
            self.bound = None
            return

        if self.bound is None:
            log.LogError('[%s] _BindCQ returned None for station=%s' % (_LOG_TAG, stationID))
            return

        try:
            joinResult = self.bound.JoinSharedQuarters()
        except Exception as e:
            log.LogException('[%s] JoinSharedQuarters: %s' % (_LOG_TAG, e))
            return

        worldSpaceID, snapshot = self._UnpackJoinResult(joinResult)
        log.LogInfo('[%s] joined ws=%s snapshot=%d' % (_LOG_TAG, worldSpaceID, len(snapshot or [])))

        for row in (snapshot or []):
            self._HandleSnapshotRow(row)

    def UpdateLocalTransform(self, x, y, z):
        """Patched CQ-room module calls this from its avatar-move tick. We
        apply a coarse rate limit so we don't flood the server with updates
        on every camera frame."""
        if self.bound is None:
            return
        now = blue.os.GetWallclockTime()
        if self.lastTransformSent is not None:
            deltaMs = blue.os.TimeDiffInMs(self.lastTransformSent, now)
            if deltaMs < self._rateLimitMs:
                return
        self.lastTransformSent = now
        try:
            self.bound.UpdateTransform(float(x), float(y), float(z))
        except Exception as e:
            log.LogException('[%s] UpdateTransform: %s' % (_LOG_TAG, e))

    # ----- notify handlers ---------------------------------------------------

    def OnCQAvatarMove(self, charID, x, y, z, worldSpaceID,
                       corpID=0, gender=0, bloodlineID=0, raceID=0):
        # Server emits this for both Join replays (initial position) and
        # live transform updates. Spawn lazily; update if already known.
        if self.localCharID is not None and charID == self.localCharID:
            return
        log.LogInfo('[%s] OnCQAvatarMove char=%s ws=%s pos=(%.2f,%.2f,%.2f) bloodline=%s race=%s' % (
            _LOG_TAG, charID, worldSpaceID, x, y, z, bloodlineID, raceID))
        if charID in self.remotes:
            self._UpdateTransform(charID, (x, y, z))
        else:
            self._Spawn(self.sceneRef, charID, (x, y, z), gender, bloodlineID, raceID, corpID)

    def OnCharNowInStation(self, *args):
        # Legacy presence notify. The 9-tuple OnCQAvatarMove replay already
        # gives us position + identity, so we treat this as informational.
        try:
            charID = self._ExtractCharID(args)
            log.LogInfo('[%s] OnCharNowInStation char=%s' % (_LOG_TAG, charID))
        except Exception:
            pass

    def OnCharNoLongerInStation(self, *args):
        try:
            charID = self._ExtractCharID(args)
        except Exception as e:
            log.LogException('[%s] OnCharNoLongerInStation: cannot extract charID (%s)' % (_LOG_TAG, e))
            return
        if charID is None:
            return
        log.LogInfo('[%s] OnCharNoLongerInStation char=%s' % (_LOG_TAG, charID))
        self._Despawn(charID)

    # ----- internal helpers --------------------------------------------------

    def _BindCQ(self, stationID):
        """Resolve and bind the captainsQuartersSvc. EVEmu registers it at
        eAccessLevel_Location, so the bind path is keyed by the solar system
        the station lives in. We pull the system ID from the session, which
        EVEmu populates as solarsystemid2 while docked."""
        machoNet = sm.GetService('machoNet')
        try:
            solarSystemID = session.solarsystemid2 or session.solarsystemid
        except Exception:
            solarSystemID = None
        if not solarSystemID:
            log.LogError('[%s] _BindCQ: no solarsystemid in session; cannot bind' % _LOG_TAG)
            return None
        addr = 'solarsystem:%d' % solarSystemID
        log.LogInfo('[%s] _BindCQ: binding captainsQuartersSvc at %s' % (_LOG_TAG, addr))
        return machoNet.GetService('captainsQuartersSvc', addr)

    def _UnpackJoinResult(self, joinResult):
        """JoinSharedQuarters historically returned (worldSpaceID, snapshot).
        Be tolerant: some Crucible RPC paths wrap the tuple in a 1-element
        outer tuple."""
        if joinResult is None:
            return (0, [])
        if isinstance(joinResult, tuple) and len(joinResult) == 2:
            return joinResult
        if isinstance(joinResult, tuple) and len(joinResult) == 1 and isinstance(joinResult[0], tuple):
            return joinResult[0]
        log.LogWarn('[%s] JoinSharedQuarters unexpected shape: %r' % (_LOG_TAG, type(joinResult)))
        return (0, list(joinResult))

    def _HandleSnapshotRow(self, row):
        """Snapshot rows are 9-tuples post-server-enrichment, but legacy
        5-tuples may still appear if you mix server versions; pad with
        zeros so a stale row never crashes the spawn path."""
        if row is None:
            return
        try:
            if len(row) == 9:
                charID, x, y, z, ws, corp, gender, bloodline, race = row
            elif len(row) == 5:
                charID, x, y, z, ws = row
                corp = gender = bloodline = race = 0
            else:
                log.LogWarn('[%s] snapshot row width=%d (expected 9 or 5): %r' %
                    (_LOG_TAG, len(row), row))
                return
        except Exception as e:
            log.LogException('[%s] snapshot row unpack: %s' % (_LOG_TAG, e))
            return

        if self.localCharID is not None and charID == self.localCharID:
            return
        self._Spawn(self.sceneRef, charID, (x, y, z), gender, bloodline, race, corp)

    def _Spawn(self, scene, charID, pos, gender, bloodlineID, raceID, corpID):
        if charID in self.remotes:
            return
        if scene is None:
            log.LogWarn('[%s] _Spawn: no scene; skipping char=%s' % (_LOG_TAG, charID))
            return
        if paperDoll is None:
            log.LogError('[%s] _Spawn: paperDoll module missing; abort spawn for char=%s' % (_LOG_TAG, charID))
            return

        try:
            dollData = sm.RemoteSvc('paperDollServer').GetPaperDollDataFor(charID)
        except Exception as e:
            log.LogException('[%s] GetPaperDollDataFor(%s): %s' % (_LOG_TAG, charID, e))
            dollData = None

        try:
            avatar = paperDoll.PaperDollAvatar(
                charID=charID,
                gender=gender,
                bloodlineID=bloodlineID,
                raceID=raceID,
                dollData=dollData,
                interactive=False,
            )
        except TypeError:
            # Older Crucible builds use positional constructors. Fall back.
            try:
                avatar = paperDoll.PaperDollAvatar(charID, gender, bloodlineID, raceID, dollData)
            except Exception as e:
                log.LogException('[%s] PaperDollAvatar fallback ctor for char=%s: %s' % (_LOG_TAG, charID, e))
                return
        except Exception as e:
            log.LogException('[%s] PaperDollAvatar(charID=%s): %s' % (_LOG_TAG, charID, e))
            return

        try:
            if hasattr(avatar, 'SetPosition'):
                avatar.SetPosition(*pos)
            elif hasattr(avatar, 'translation'):
                avatar.translation = pos
        except Exception as e:
            log.LogException('[%s] SetPosition char=%s: %s' % (_LOG_TAG, charID, e))

        try:
            if hasattr(scene, 'AddEntity'):
                scene.AddEntity(avatar)
            elif hasattr(scene, 'AddDynamic'):
                scene.AddDynamic(avatar)
        except Exception as e:
            log.LogException('[%s] scene.AddEntity char=%s: %s' % (_LOG_TAG, charID, e))
            self._SafeUnload(charID, avatar)
            return

        self.remotes[charID] = avatar
        log.LogInfo('[%s] spawned remote avatar char=%s race=%s bloodline=%s gender=%s pos=%s' %
            (_LOG_TAG, charID, raceID, bloodlineID, gender, pos))

    def _UpdateTransform(self, charID, pos):
        avatar = self.remotes.get(charID)
        if avatar is None:
            return
        try:
            if hasattr(avatar, 'SetPosition'):
                avatar.SetPosition(*pos)
            elif hasattr(avatar, 'translation'):
                avatar.translation = pos
        except Exception as e:
            log.LogException('[%s] _UpdateTransform char=%s: %s' % (_LOG_TAG, charID, e))

    def _Despawn(self, charID):
        avatar = self.remotes.pop(charID, None)
        if avatar is None:
            return
        self._SafeUnload(charID, avatar)
        log.LogInfo('[%s] despawned remote avatar char=%s' % (_LOG_TAG, charID))

    def _SafeUnload(self, charID, avatar):
        try:
            if hasattr(avatar, 'Unload'):
                avatar.Unload()
            elif hasattr(avatar, 'Disable'):
                avatar.Disable()
        except Exception as e:
            log.LogException('[%s] avatar.Unload char=%s: %s' % (_LOG_TAG, charID, e))

    def _ExtractCharID(self, args):
        """OnCharNowInStation / OnCharNoLongerInStation in Crucible carry a
        nested tuple. Walk into it until we get an int."""
        cur = args
        for _ in xrange(4):
            if cur is None:
                return None
            if isinstance(cur, (int, long)):
                return cur
            if isinstance(cur, (tuple, list)) and len(cur) > 0:
                cur = cur[0]
            else:
                return None
        return None
