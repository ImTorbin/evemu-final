# -*- coding: utf-8 -*-
# Phase 1 + Phase 2 client-side LiveUpdate patches for multiplayer
# Captain's Quarters.
#
# Compiled with Python 2.7 by build_phase1_sql.py and shipped as marshaled
# func_code blobs in the `liveupdates` table. The Crucible client's
# liveUpdateSvc replaces the matching method's func_code with these blobs
# at handshake time (see liveUpdateSvc.OnLiveClientUpdate, codeType
# 'globalClassMethod').
#
# When liveUpdateSvc replaces a method with marshaled `func_code`, those
# functions resolve global names in the *host* module (e.g. cqview, cef, not
# this file).  Only imports and locals from the host + session/sm/cfg/const
# are safe.  Cross-hook state lives on __builtin__ as `_EVEMuCQStashV1`:
#   [0] = CQ notify helper or None,  [1] = {entityID: speed EMA float},
#   [2] = set of entityID ints for *remote* avatars the server says are
#   seated (AO manager never marks remotes, so Biped must not use
#   IsEntityUsingActionObject for sit locomotion).
#
# Functions in this file map to LiveUpdate rows like so (see
# build_phase1_sql.py for the SQL):
#
#   _GoStation_patched               -> viewstate.CQView::CQView._GoStation
#   UnloadView_patched               -> viewstate.CQView::CQView.UnloadView
#   ClientOnlyPlayerSpawner_init_patched
#       -> cef.ClientOnlyPlayerSpawner::ClientOnlyPlayerSpawner.__init__
#   ClientOnlyPlayerSpawner_GetRecipe_patched
#       -> cef.ClientOnlyPlayerSpawner::ClientOnlyPlayerSpawner.GetRecipe
#
# All Phase 1/2 hooks on CQView are wrapped in try/except so any failure
# here cannot break station entry or exit.

# Phase 3: emote menu (right-click self in CQ) + server broadcast. Indices
# 0..7 must match src/eve-server/system/CQService.cpp (kMaxBroadcastEmote=8)
# and the morpheme string must exist in the avatar requestIDs (see log on
# first sit) — edit labels/names to match your client build.
CQ_EMOTE_MENU = [
    ('Wave', 'emote_wave'),
    ('Bow', 'emote_bow'),
    ('Applause', 'emote_clap'),
    ('Dance', 'emote_dance'),
    ('Point', 'emote_point'),
    ('Salute', 'emote_salute'),
    ('Fidget', 'fidget_01'),
    ('Stretch', 'fidget_02'),
]

def _cq_resolve_remote_char_from_entity_pick(helper, picked_eid):
    """
    _PickObject often hits a child mesh, not the root player entity. Walk parents
    until the id matches a registered CQ remote or we leave the tree.
    """
    if not helper or picked_eid is None:
        return None
    try:
        eid0 = int(picked_eid)
    except Exception:
        return None
    p2c = getattr(helper, '_pickEntityToChar', None)
    try:
        for _step in range(64):
            if p2c:
                try:
                    c = p2c.get(int(eid0))
                    if c is not None:
                        return int(c)
                except Exception:
                    sys.exc_clear()
            for cid, root in helper._avatars.iteritems():
                if root is not None and int(root) == eid0:
                    return int(cid)
            ec = sm.GetService('entityClient')
            ent = ec.FindEntityByID(eid0)
            if ent is None:
                return None
            parent = getattr(ent, 'parent', None)
            if parent is None:
                gp = getattr(ent, 'GetParent', None)
                if gp is not None and callable(gp):
                    try:
                        parent = gp()
                    except Exception:
                        sys.exc_clear()
            if parent is None:
                return None
            p_eid = getattr(parent, 'entityID', None)
            if p_eid is None:
                return None
            eid0 = int(p_eid)
    except Exception:
        sys.exc_clear()
    return None


# ---------------------------------------------------------------------------
# Patch A: viewstate.CQView::CQView._GoStation
# ---------------------------------------------------------------------------
def _GoStation_patched(self, change):
    if 'stationid' in change:
        enteringStationLabel = localization.GetByLabel('UI/Station/EnteringStation')
        clearingCurrentStateLabel = localization.GetByLabel('UI/Station/ClearingCurrentState')
        self.loading.ProgressWnd(enteringStationLabel, '', 1, 5)
        self.loading.ProgressWnd(enteringStationLabel, clearingCurrentStateLabel, 2, 5)
        fromstation, tostation = change['stationid']
        self.loading.ProgressWnd(enteringStationLabel, cfg.evelocations.Get(tostation).name, 3, 5)
        if tostation is not None and fromstation != tostation:
            self.stationID = tostation
            setupStationLabel = localization.GetByLabel('UI/Station/SetupStation', stationName=cfg.evelocations.Get(tostation).name)
            self.loading.ProgressWnd(enteringStationLabel, setupStationLabel, 4, 5)
            self.LoadHangarBackground()
            self.station.CleanUp()
            self.station.StopAllStationServices()
            self.station.Setup()
            if settings.user.ui.Get('doIntroTutorial%s' % session.charid, 0):
                tutID = uix.tutorialTutorials
            else:
                tutID = uix.tutorialWorldspaceNavigation
            uthread.new(self.OpenStationTutorial_thread, tutID)
        elif tostation is None:
            self.station.CleanUp()
        doneLabel = localization.GetByLabel('UI/Common/Done')
        self.loading.ProgressWnd(enteringStationLabel, doneLabel, 5, 5)
    else:
        self.station.CheckSession(change)
    try:
        ws = session.worldspaceid
        st = session.stationid
        log.LogInfo('[CQ-P2] _GoStation hook ws=', ws, 'st=', st)
        if not ws:
            log.LogInfo('[CQ-P2] no worldspaceid set, skipping bind')
            return
        if not getattr(self, '_cqPhase1Helper', None):
            view_ref = self
            # Morpheme names: local tuple so nested class methods (cqview
            # globals) do not reference CQ_EMOTE_MENU from this file.
            _em_menu = (('Wave', 'emote_wave'),
             ('Bow', 'emote_bow'),
             ('Applause', 'emote_clap'),
             ('Dance', 'emote_dance'),
             ('Point', 'emote_point'),
             ('Salute', 'emote_salute'),
             ('Fidget', 'fidget_01'),
             ('Stretch', 'fidget_02'))

            class _CQPhase1Helper(object):
                __notifyevents__ = ['OnCharNowInStation', 'OnCharNoLongerInStation',
                                    'OnCQAvatarMove', 'OnCQAvatarAction', 'OnCQAvatarEmote',
                                    'OnCQCustomAgentGone']

                def __init__(self):
                    self._avatars = {}
                    self._view = view_ref
                    # Phase 2b: own-transform broadcast.
                    # _moniker is the bound CQ proxy for UpdateTransform calls.
                    # _stop signals the tick uthreads to exit on UnloadView.
                    # _lastSentPos / _lastSentYaw are what we last shipped,
                    #   so we can skip duplicate sends when the avatar is idle.
                    # Phase 2c: receive-side smoothing.
                    # _targets[charID]    = (x,y,z) latest authoritative pos
                    # Phase 2d: rotation sync.
                    # _targetsYaw[charID] = yaw radians (rotation around vertical
                    #   axis, all that matters in CQ since avatars stay upright).
                    #   The interp tick eases each remote avatar's
                    #   PositionComponent toward its target every frame instead
                    #   of snapping on every OnCQAvatarMove.
                    # Phase 2f: ActionObject sync (couch sit + future emotes).
                    # _lastSentAction = last (uid, stationIdx) we shipped from
                    #   the local poll tick. (0, -1) means standing.
                    # _remoteActions[charID] = (uid, stationIdx) latest known
                    #   action state for a remote. Stored separately from
                    #   _avatars because OnCQAvatarAction can arrive before the
                    #   avatar entity is finished spawning.
                    self._moniker = None
                    self._stop = False
                    self._lastSentPos = None
                    self._lastSentYaw = None
                    self._lastSentAction = (0, -1)
                    self._targets = {}
                    self._targetsYaw = {}
                    self._remoteActions = {}
                    # Phase 2g: per-charID diagnostic flag so we dump the
                    # morpheme network's available request names exactly once
                    # per remote (one big log line on first sit/stand attempt).
                    # Bytecode dispatch on this client doesn't expose request
                    # names anywhere else, so logging them ourselves is the
                    # only way to learn the right BroadcastRequest IDs.
                    self._animReqsLogged = {}
                    # charID -> (aoUID, stationIdx) last time we fired sit/stand
                    # morpheme for that remote (avoid duplicate BroadcastRequest).
                    self._lastRemoteActMorpheme = {}
                    # charIDs: one frame, skip position lerp so sit socket snap
                    # (see _GetRemoteSitWorldPose) does not fight the interp tick.
                    self._oneShotPosSnap = set()
                    # Last seat (aoUID, stationIdx) after a successful *pose* write;
                    # morpheme success is tracked separately — stand exit needs the seat.
                    self._lastRemoteSitSeat = {}
                    # root entityID -> charID for direct ray hits; sub-meshes use parent-walk in resolve.
                    self._pickEntityToChar = {}

                def Spawn(self, charID, position, yaw=0.0, appearanceOverride=0):
                    # `self._avatars[charID]` invariants:
                    #   key absent  -> no entity, free to spawn
                    #   value None  -> spawn coroutine in flight (yielded on RPC)
                    #   value int   -> entity is registered with that entityID
                    # OnCharNowInStation + OnCQAvatarMove can both arrive for
                    # the same charID and both yield on GetPaperDollDataFor;
                    # the sentinel prevents a duplicate scene registration
                    # (which used to fire "Adding entity ... already in this
                    # scene" stack traces).
                    if not charID or charID == session.charid:
                        return
                    if charID in self._avatars:
                        return
                    self._avatars[charID] = None
                    try:
                        import cef
                        ent_client = self._view.entityClient
                        scene = ent_client.GetEntityScene(self._view.stationID)
                        if scene is None:
                            log.LogError('[CQ-P2] Spawn: no scene for stationID=', self._view.stationID)
                            del self._avatars[charID]
                            return
                        # Build the spawn-time quaternion so the remote avatar
                        # appears facing the right way the moment it pops in
                        # (no visible 1-frame snap from yaw=0 to authoritative).
                        rotation = geo2.QuaternionRotationSetYawPitchRoll(float(yaw), 0.0, 0.0)
                        # instance charID may be synthetic (0xC0******); paper doll from template char
                        doll_id = int(appearanceOverride) if appearanceOverride else int(charID)
                        paperdolldna = sm.RemoteSvc('paperDollServer').GetPaperDollDataFor(doll_id)
                        pubInfo = sm.RemoteSvc('charMgr').GetPublicInfo(doll_id)
                        if charID not in self._avatars:
                            log.LogInfo('[CQ-P2] Spawn: aborted while RPC in flight charID=', charID)
                            return
                        if paperdolldna is None or pubInfo is None:
                            log.LogError('[CQ-P2] Spawn: missing paperdolldna or pubInfo charID=', charID)
                            del self._avatars[charID]
                            return
                        spawner = cef.ClientOnlyPlayerSpawner(scene.sceneID, charID, position, rotation, paperdolldna, pubInfo)
                        entity = self._view.entitySpawnClient.LoadEntityFromSpawner(spawner)
                        if entity is not None:
                            eid = getattr(entity, 'entityID', charID)
                            self._avatars[charID] = eid
                            try:
                                self._pickEntityToChar[int(eid)] = int(charID)
                            except Exception:
                                sys.exc_clear()
                            # Seed interp targets so the avatar starts at rest
                            # at its spawn point/orientation until the next
                            # OnCQAvatarMove.
                            self._targets[charID] = (float(position[0]), float(position[1]), float(position[2]))
                            self._targetsYaw[charID] = float(yaw)
                            log.LogInfo('[CQ-P2] spawned remote avatar charID=', charID, 'entityID=', eid, 'yaw=', yaw)
                            # Phase 2f: replay any pending action state that
                            # arrived (via OnCQAvatarAction or snapshot replay)
                            # while the spawn was in flight. Without this the
                            # avatar would pop in standing even if the server
                            # said they are sitting.
                            if charID in self._remoteActions:
                                self._ApplyRemoteAction(charID)
                        else:
                            log.LogError('[CQ-P2] Spawn: LoadEntityFromSpawner returned None charID=', charID)
                            if charID in self._avatars and self._avatars[charID] is None:
                                del self._avatars[charID]
                    except Exception as e:
                        log.LogException('[CQ-P2] Spawn error charID=' + str(charID) + ': ' + str(e))
                        sys.exc_clear()
                        if charID in self._avatars and self._avatars[charID] is None:
                            del self._avatars[charID]

                def Update(self, charID, position, yaw=0.0, appearanceOverride=0):
                    # Phase 2c/d: never snap-write the entity here. Just record
                    # the new authoritative position+yaw target;
                    # _InterpolationTick eases the avatar toward it each
                    # render-rate frame, which is what makes movement and
                    # rotation smooth instead of jerky.
                    if not charID or charID == session.charid:
                        return
                    try:
                        # Phase 2i: While the remote is on an ActionObject (couch
                        # seat), their OnCQAvatarMove stream still reflects the
                        # last *walking* sample (the server has no zaction to move
                        # them). If we keep writing that into _targets, the
                        # interp tick drags the avatar back to a spot in front
                        # of the couch and kills the sit pose + zaction. Ignore
                        # move broadcasts until they stand; targets are driven by
                        # _ApplyRemoteAction (cfg seat pose + morpheme, no zaction).
                        ract = self._remoteActions.get(charID, (0, -1))
                        if ract[0] and ract[1] >= 0:
                            if charID not in self._avatars:
                                self.Spawn(charID, position, yaw, appearanceOverride)
                            return
                        target = (float(position[0]), float(position[1]), float(position[2]))
                        self._targets[charID] = target
                        self._targetsYaw[charID] = float(yaw)
                        if charID not in self._avatars:
                            self.Spawn(charID, position, yaw, appearanceOverride)
                            return
                        eid = self._avatars[charID]
                        if eid is None:
                            return
                        entity = self._view.entityClient.FindEntityByID(eid)
                        if entity is None:
                            del self._avatars[charID]
                            self.Spawn(charID, position, yaw, appearanceOverride)
                            return
                    except Exception as e:
                        log.LogException('[CQ-P2] Update error charID=' + str(charID) + ': ' + str(e))
                        sys.exc_clear()

                def Despawn(self, charID):
                    if charID not in self._avatars:
                        if charID in self._targets:
                            del self._targets[charID]
                        if charID in self._targetsYaw:
                            del self._targetsYaw[charID]
                        if charID in self._remoteActions:
                            del self._remoteActions[charID]
                        return
                    try:
                        eid = self._avatars[charID]
                        try:
                            if eid is not None:
                                self._pickEntityToChar.pop(int(eid), None)
                        except Exception:
                            sys.exc_clear()
                        del self._avatars[charID]
                        if charID in self._targets:
                            del self._targets[charID]
                        if charID in self._targetsYaw:
                            del self._targetsYaw[charID]
                        if charID in self._remoteActions:
                            del self._remoteActions[charID]
                        if charID in self._animReqsLogged:
                            del self._animReqsLogged[charID]
                        if charID in self._lastRemoteActMorpheme:
                            del self._lastRemoteActMorpheme[charID]
                        if charID in self._lastRemoteSitSeat:
                            del self._lastRemoteSitSeat[charID]
                        try:
                            self._oneShotPosSnap.discard(charID)
                        except Exception:
                            sys.exc_clear()
                        if eid is None:
                            log.LogInfo('[CQ-P2] Despawn: cancelled in-flight spawn charID=', charID)
                            return
                        try:
                            b = __import__('__builtin__')
                            M = getattr(b, '_EVEMuCQStashV1', None)
                            if M is not None:
                                M[1].pop(int(eid), None)
                                if len(M) > 2:
                                    M[2].discard(int(eid))
                        except Exception:
                            sys.exc_clear()
                        # Best-effort: clear any AO occupancy before deleting
                        # the entity so the seat shows free for everyone.
                        try:
                            ao_svc = sm.GetService('actionObjectClientSvc')
                            if ao_svc is not None and getattr(ao_svc, 'manager', None) is not None:
                                ao_svc.manager.StopUsingActionObject(eid)
                        except Exception:
                            sys.exc_clear()
                        self._view.entityClient.UnregisterAndDestroyEntityByID(eid)
                        log.LogInfo('[CQ-P2] despawned remote avatar charID=', charID, 'entityID=', eid)
                    except Exception as e:
                        log.LogException('[CQ-P2] Despawn error charID=' + str(charID) + ': ' + str(e))
                        sys.exc_clear()

                def DespawnAll(self):
                    for cid in list(self._avatars.keys()):
                        self.Despawn(cid)

                def OnCharNowInStation(self, *a, **kw):
                    log.LogInfo('[CQ-P2] OnCharNowInStation a=', a, 'kw=', kw)
                    try:
                        rec = a[0] if a else None
                        if not rec:
                            return
                        cid = rec[0]
                        if not cid or cid == session.charid:
                            return
                        self.Spawn(cid, (0.0, 0.0, 0.0))
                    except Exception as e:
                        log.LogException('[CQ-P2] OnCharNowInStation handler error: ' + str(e))
                        sys.exc_clear()

                def OnCharNoLongerInStation(self, *a, **kw):
                    log.LogInfo('[CQ-P2] OnCharNoLongerInStation a=', a, 'kw=', kw)
                    try:
                        rec = a[0] if a else None
                        if not rec:
                            return
                        cid = rec[0]
                        if not cid:
                            return
                        self.Despawn(cid)
                    except Exception as e:
                        log.LogException('[CQ-P2] OnCharNoLongerInStation handler error: ' + str(e))
                        sys.exc_clear()

                def OnCQCustomAgentGone(self, *a, **kw):
                    try:
                        if len(a) < 1:
                            return
                        cid = int(a[0])
                        if not cid:
                            return
                        self.Despawn(cid)
                    except Exception as e:
                        log.LogException('[CQ-P2] OnCQCustomAgentGone: ' + str(e))
                        sys.exc_clear()

                def OnCQAvatarMove(self, *a, **kw):
                    try:
                        if len(a) < 4:
                            return
                        cid = a[0]
                        if not cid or cid == session.charid:
                            return
                        position = (float(a[1]), float(a[2]), float(a[3]))
                        # Phase 2d: payload index 9 carries yaw. Default 0
                        # for backward-compat with any in-flight legacy
                        # 9-tuple notifies during a server hot-swap.
                        yaw = float(a[9]) if len(a) >= 10 else 0.0
                        # Index 10: paper-doll source char (0 = use cid). Station CQ custom agents.
                        app = int(a[10]) if len(a) >= 11 else 0
                        self.Update(cid, position, yaw, app)
                    except Exception as e:
                        log.LogException('[CQ-P2] OnCQAvatarMove handler error: ' + str(e))
                        sys.exc_clear()

                # ---- Phase 2f: action object (couch sit / etc) sync ---------
                # OnCQAvatarAction payload: (charID, actionObjectUID, stationIdx)
                # stationIdx = -1 means "stood up / no action". We always track
                # the latest state in _remoteActions so the value survives
                # spawn coroutine yields and is re-applied on respawn.
                def OnCQAvatarAction(self, *a, **kw):
                    try:
                        if len(a) < 3:
                            return
                        cid = a[0]
                        if not cid or cid == session.charid:
                            return
                        uid = int(a[1])
                        idx = int(a[2])
                        log.LogInfo('[CQ-P2f] OnCQAvatarAction char=', cid, 'aoUID=', uid, 'idx=', idx)
                        self._remoteActions[cid] = (uid, idx)
                        self._ApplyRemoteAction(cid)
                    except Exception as e:
                        log.LogException('[CQ-P2f] OnCQAvatarAction handler error: ' + str(e))
                        sys.exc_clear()

                # ---- Phase 2f helpers ---------------------------------------
                # Walk the CQ scene's entities and return the (entity, component)
                # pair whose actionObjectData.UID matches `uid`. Cached lookup
                # would be nice but couch entities outlive the helper, so a
                # one-shot scan on each apply is fine -- this only runs on
                # discrete state changes, not every frame.
                def _FindActionObjectByUID(self, uid):
                    try:
                        ent_client = getattr(self._view, 'entityClient', None)
                        if ent_client is None:
                            return (None, None)
                        scene = ent_client.GetEntityScene(self._view.stationID)
                        if scene is None or not hasattr(scene, 'entities'):
                            return (None, None)
                        for entityID, entity in scene.entities.iteritems():
                            try:
                                comp = entity.GetComponent('actionObject')
                            except Exception:
                                comp = None
                                sys.exc_clear()
                            if comp is None:
                                continue
                            ao_data = getattr(comp, 'actionObjectData', None)
                            if ao_data is None:
                                continue
                            if int(ao_data.UID) == int(uid):
                                return (entity, comp)
                        return (None, None)
                    except Exception as e:
                        log.LogException('[CQ-P2f] _FindActionObjectByUID error: ' + str(e))
                        sys.exc_clear()
                        return (None, None)

                # World (x,y,z) + yaw for a remote avatar sitting in
                # actionStations[station_idx], without running zaction on the
                # observer. zaction/StartAction re-targets the *local* player's
                # camera to the action object, which is why B/C would snap
                # onto A; we only need the cushion pose for interp + anims.
                def _GetRemoteSitWorldPose(self, ao_entity, ao_comp, station_idx):
                    # Seat world pose for the observer's copy of the couch.
                    #
                    # IMPORTANT: actionStations[i].worldPosition (and similar)
                    # is often the *approach / interact* point in front of the
                    # furniture, not the seated cushion — remotes then appear to
                    # stand where the placer would start the sit zaction. The
                    # static cfg row (posX/Y/Z in object space) matches the
                    # actual sit socket, so try that first; only then fall
                    # back to runtime getters.
                    stations = getattr(ao_comp, 'actionStations', None) or []
                    if station_idx < 0 or station_idx >= len(stations):
                        return None
                    st = stations[station_idx]
                    try:
                        ao_data = getattr(ao_comp, 'actionObjectData', None)
                        if ao_data is not None:
                            aoid = int(getattr(ao_data, 'UID', 0) or 0)
                            if aoid:
                                rows = cfg.actionObjectStations.get(aoid) or []
                                rows = list(rows)
                                station_row = None
                                if 0 <= station_idx < len(stations):
                                    st0 = stations[station_idx]
                                    iid = (getattr(st0, 'actionStationInstanceID', None) or
                                           getattr(st0, 'instanceID', None) or
                                           getattr(st0, 'InstID', None))
                                    if iid is not None:
                                        for r in rows:
                                            if int(r.actionStationInstanceID) == int(iid):
                                                station_row = r
                                                break
                                try:
                                    rows.sort(key=lambda r: r.actionStationInstanceID)
                                except Exception:
                                    sys.exc_clear()
                                if station_row is None and 0 <= station_idx < len(rows):
                                    station_row = rows[station_idx]
                                if station_row is not None:
                                    lp = (float(station_row.posX), float(station_row.posY), float(station_row.posZ))
                                    lq = geo2.QuaternionRotationSetYawPitchRoll(
                                        float(station_row.rotY), float(station_row.rotX), float(station_row.rotZ)
                                    )
                                    pc_ao = ao_entity.GetComponent('position')
                                    if pc_ao is not None:
                                        ao_p = pc_ao.position
                                        ao_r = pc_ao.rotation
                                        if ao_r is None:
                                            wpos = geo2.Vec3Add(ao_p, lp)
                                            wrot = lq
                                        else:
                                            t = geo2.QuaternionTransformVector(ao_r, lp)
                                            wpos = geo2.Vec3Add(t, ao_p)
                                            wrot = geo2.QuaternionMultiply(ao_r, lq)
                                        ypr = geo2.QuaternionRotationGetYawPitchRoll(wrot)
                                        return (float(wpos[0]), float(wpos[1]), float(wpos[2]), float(ypr[0]))
                    except Exception as e:
                        log.LogException('[CQ-P2l] _GetRemoteSitWorldPose (cfg first): ' + str(e))
                        sys.exc_clear()
                    try:
                        wtp = None
                        for st_name in ('worldPosition', 'worldTranslation', 'GetWorldPosition'):
                            cand = getattr(st, st_name, None)
                            if cand is not None and callable(cand):
                                try:
                                    cand = cand()
                                except Exception:
                                    sys.exc_clear()
                                    cand = None
                            if cand is not None and len(cand) >= 3:
                                wtp = cand
                                break
                        if wtp is not None and len(wtp) >= 3:
                            wtr = None
                            for st_name in ('worldRotation', 'GetWorldRotation'):
                                cand = getattr(st, st_name, None)
                                if cand is not None and callable(cand):
                                    try:
                                        cand = cand()
                                    except Exception:
                                        sys.exc_clear()
                                        cand = None
                                if cand is not None:
                                    wtr = cand
                                    break
                            if wtr is not None:
                                ypr = geo2.QuaternionRotationGetYawPitchRoll(wtr)
                            else:
                                pco = ao_entity.GetComponent('position')
                                rot = pco.rotation if pco is not None else None
                                if rot is not None:
                                    ypr = geo2.QuaternionRotationGetYawPitchRoll(rot)
                                else:
                                    ypr = (0.0, 0.0, 0.0)
                            log.LogInfo('[CQ-P2p] _GetRemoteSitWorldPose: using runtime fallback (cfg path missed)')
                            return (float(wtp[0]), float(wtp[1]), float(wtp[2]), float(ypr[0]))
                    except Exception:
                        sys.exc_clear()
                    return None

                # Floor "use" / approach point in front of the seat (runtime
                # worldPosition). _GetRemoteSitWorldPose prefers cfg *socket* for
                # the cushioned sit pose; for stand-up we must move the remote's
                # root here or the stand clip's root motion plus cushion
                # PositionComponent reads as floating in front of the furniture.
                def _GetRemoteApproachWorldPose(self, ao_entity, ao_comp, station_idx):
                    stations = getattr(ao_comp, 'actionStations', None) or []
                    if station_idx < 0 or station_idx >= len(stations):
                        return None
                    st = stations[station_idx]
                    try:
                        wtp = None
                        for st_name in ('worldPosition', 'worldTranslation', 'GetWorldPosition'):
                            cand = getattr(st, st_name, None)
                            if cand is not None and callable(cand):
                                try:
                                    cand = cand()
                                except Exception:
                                    sys.exc_clear()
                                    cand = None
                            if cand is not None and len(cand) >= 3:
                                wtp = cand
                                break
                        if wtp is not None and len(wtp) >= 3:
                            wtr = None
                            for st_name in ('worldRotation', 'GetWorldRotation'):
                                cand = getattr(st, st_name, None)
                                if cand is not None and callable(cand):
                                    try:
                                        cand = cand()
                                    except Exception:
                                        sys.exc_clear()
                                        cand = None
                                if cand is not None:
                                    wtr = cand
                                    break
                            if wtr is not None:
                                ypr = geo2.QuaternionRotationGetYawPitchRoll(wtr)
                            else:
                                pco = ao_entity.GetComponent('position')
                                rot = pco.rotation if pco is not None else None
                                if rot is not None:
                                    ypr = geo2.QuaternionRotationGetYawPitchRoll(rot)
                                else:
                                    ypr = (0.0, 0.0, 0.0)
                            return (float(wtp[0]), float(wtp[1]), float(wtp[2]), float(ypr[0]))
                    except Exception as e:
                        log.LogException('[CQ-P2v] _GetRemoteApproachWorldPose: ' + str(e))
                        sys.exc_clear()
                    return None

                # Phase 2g: directly drive the remote avatar's morpheme
                # animation network. The action tree's PerformAnim proc fires
                # a Trinity scene event (e.g. "cq_couch_sit_down_play to game
                # object 24") that local PaperDollAvatar models hook for sit
                # transitions, but on a remote avatar the same proc evidently
                # does not reach the morpheme state machine -- the avatar
                # warps to the seat and never plays the sit animation. So we
                # ALSO call BroadcastRequest directly on the remote avatar's
                # AnimationController, which routes straight into the
                # morpheme network via animationNetwork.BroadcastRequestByID.
                # We don't know the morpheme request ID names a priori, so:
                #   1) on first call per charID, dump all available request
                #      names to the log so a future patch can target a
                #      specific one with confidence;
                #   2) try a list of plausible candidates and stop at the
                #      first one that actually exists in this network's
                #      requestIDs map (BroadcastRequest no-ops with a single
                #      LogError if the name is unknown, so trying several is
                #      safe even when most are wrong).
                def _DriveRemoteAnim(self, charID, candidates, label, station_idx=None):
                    try:
                        eid = self._avatars.get(charID)
                        if eid is None:
                            return False
                        ent_client = getattr(self._view, 'entityClient', None)
                        if ent_client is None:
                            return False
                        avatar = ent_client.FindEntityByID(eid)
                        if avatar is None:
                            return False
                        anim_comp = avatar.GetComponent('animation')
                        if anim_comp is None:
                            log.LogError('[CQ-P2g] _DriveRemoteAnim: no animation component charID=', charID,
                                         'label=', label)
                            return False
                        controller = getattr(anim_comp, 'controller', None)
                        if controller is None:
                            log.LogError('[CQ-P2g] _DriveRemoteAnim: no controller charID=', charID,
                                         'label=', label)
                            return False
                        req_ids = getattr(controller, 'requestIDs', None)
                        if req_ids is None:
                            log.LogError('[CQ-P2g] _DriveRemoteAnim: no requestIDs map charID=', charID)
                            return False
                        # Fuzzy + "name in map" need string keys. If the graph only
                        # exposes numeric request IDs, Trinity event strings like
                        # cq_couch_sit_down_play are NOT valid BroadcastRequest
                        # names (client logs: "does not exist!") and blind-spam
                        # triggers move-mode warnings — do not call unknown names.
                        str_keys = []
                        try:
                            for _k in req_ids:
                                if isinstance(_k, basestring):
                                    str_keys.append(_k)
                        except Exception:
                            sys.exc_clear()
                        try:
                            n_all = len(req_ids)
                        except Exception:
                            n_all = 0
                            sys.exc_clear()
                        if n_all and not str_keys:
                            log.LogError('[CQ-P2g] _DriveRemoteAnim: requestIDs have no string keys; cannot match couch names charID=', charID,
                                         ' label=', label, ' n=', n_all)
                            return False
                        # First-time diagnostic dump per remote so we can
                        # discover the actual sit/stand request names from a
                        # single test session.
                        if not self._animReqsLogged.get(charID):
                            self._animReqsLogged[charID] = True
                            try:
                                names = sorted(str_keys)
                                log.LogInfo('[CQ-P2g] morpheme string requestIDs for charID=', charID,
                                            ' count=', len(names), ' (Trinity *Play* names are often not in this set)')
                                # Chunk to avoid one absurdly long line in the
                                # binary log; 8 names per line stays readable
                                # in lbw text dumps.
                                CHUNK = 8
                                i = 0
                                while i < len(names):
                                    log.LogInfo('[CQ-P2g] reqs[', i, ':]=', names[i:i + CHUNK])
                                    i += CHUNK
                            except Exception:
                                sys.exc_clear()
                        # Find any candidate that exists. Order matters: we
                        # list more specific names first so e.g.
                        # 'cq_couch_sit_down' is preferred over a generic
                        # 'sit_down' if both exist.
                        chosen = None
                        for name in candidates:
                            if name in req_ids:
                                chosen = name
                                break
                        # Crucible Biped requestIDs often differ from the legacy
                        # zaction/Trinity event names; try substring match on the
                        # real network (sittingtest2.lbw: hardcoded stand list 0/10).
                        if chosen is None and label in ('sit', 'stand'):
                            try:
                                nml = list(str_keys) if str_keys else []
                            except Exception:
                                nml = []
                                sys.exc_clear()
                            # Prefer L/C/R couch clips when the server reports a
                            # specific cushion index (Crucible uses different
                            # *_l_* / *_c_* / *_r_* request names; sit test 3
                            # with idx=2 missed center-only 'cq_couch_sit_down_play').
                            try:
                                if label == 'sit' and station_idx in (0, 1, 2) and nml:
                                    tag = ('_l_', '_c_', '_r_')[station_idx]
                                    nml = sorted(
                                        nml,
                                        key=lambda s, t=tag: (0 if t in s.lower() else 1, -len(s), s),
                                    )
                                else:
                                    nml = sorted(nml, key=lambda s: (-len(s), s))
                            except Exception:
                                sys.exc_clear()
                            try:
                                for nm in nml:
                                    low = nm.lower()
                                    if label == 'sit':
                                        # Reject stand-up / rise clips that still contain
                                        # "sit" as a substring in some names.
                                        if (('couch_stand' in low or 'stand_up' in low
                                             or 'standup' in low) and 'sit_down' not in low
                                            and 'sitdown' not in low and 'couch_sit' not in low):
                                            continue
                                        for s in (
                                                'sit_down_play', 'couch_sit_down',
                                                '_l_play', '_c_play', '_r_play', 'sit_l', 'sit_c', 'sit_r',
                                                'couch_sit', 'couch sit', 'sit_down', 'sitdown',
                                                'sitting', ' sit', 'sit', 'seat', 'cushion',
                                        ):
                                            if s in low:
                                                chosen = nm
                                                break
                                    else:
                                        # Stand / get up (include *_play; see sit_cands).
                                        for s in (
                                                'stand_up_play', 'couch_stand_up',
                                                'couch_stand', 'couch stand', 'stand_up', 'standup',
                                                'get_up', 'getup', 'couchstand',
                                        ):
                                            if s in low and 'sit_down' not in low and 'sitdown' not in low:
                                                chosen = nm
                                                break
                                    if chosen:
                                        break
                            except Exception:
                                sys.exc_clear()
                        if chosen is None:
                            if label in ('sit', 'stand'):
                                log.LogError('[CQ-P2g] _DriveRemoteAnim: no couch/stand name in string requestIDs charID=',
                                             charID, ' label=', label, ' (pose-only; see P2g string dump above)')
                            else:
                                log.LogError('[CQ-P2g] _DriveRemoteAnim: none of ', candidates,
                                             ' (and no fuzzy) charID=', charID, 'label=', label)
                            return False
                        try:
                            if label in ('sit', 'stand') and chosen is not None:
                                log.LogInfo('[CQ-P2j] BroadcastRequest charID=', charID,
                                            'label=', label, 'name=', chosen,
                                            ' fuzzy=', chosen not in candidates)
                            else:
                                log.LogInfo('[CQ-P2g] BroadcastRequest charID=', charID,
                                            'label=', label, 'name=', chosen)
                            controller.BroadcastRequest(chosen)
                            return True
                        except Exception as be:
                            log.LogException('[CQ-P2g] BroadcastRequest error: ' + str(be))
                            sys.exc_clear()
                            return False
                    except Exception as e:
                        log.LogException('[CQ-P2g] _DriveRemoteAnim error charID=' + str(charID) + ': ' + str(e))
                        sys.exc_clear()
                        return False

                # Phase 2l/2m: cfg seat *pose*; phase 2q: sit/stand *morpheme* on the
                # remote's AnimationController (BroadcastRequest on that entity
                # only). Without a sit request, the mesh stays in default stand
                # idle on the cushion. We still do NOT start zaction / AO sit on
                # the remote (those steal camera on observers in some builds).
                def _ApplyRemoteAction(self, charID):
                    try:
                        eid = self._avatars.get(charID)
                        # Avatar entity not yet spawned (or in-flight sentinel) --
                        # re-apply once Spawn finishes.
                        if eid is None:
                            return
                        action = self._remoteActions.get(charID, (0, -1))
                        uid, idx = action
                        ent_client = getattr(self._view, 'entityClient', None)
                        if ent_client is None:
                            log.LogError('[CQ-P2f] _ApplyRemoteAction: no entityClient on view')
                            return
                        avatar = ent_client.FindEntityByID(eid)
                        if avatar is None:
                            log.LogError('[CQ-P2f] _ApplyRemoteAction: avatar entity gone for charID=', charID)
                            return
                        if avatar.GetComponent('action') is None:
                            log.LogInfo('[CQ-P2l] remote avatar (no zaction) charID=', charID)

                        # Stand: if they were on a seat, request stand-up morpheme so
                        # we do not stay in a sit clip off the furniture.
                        if idx < 0 or uid == 0:
                            try:
                                b0 = __import__('__builtin__')
                                M0 = getattr(b0, '_EVEMuCQStashV1', None)
                                if M0 is not None and len(M0) > 2:
                                    M0[2].discard(int(eid))
                            except Exception:
                                sys.exc_clear()
                            seat_rec = (self._lastRemoteSitSeat.get(charID) or
                                        self._lastRemoteActMorpheme.get(charID) or
                                        (0, -1))
                            prev = self._lastRemoteActMorpheme.get(charID, (0, -1))
                            puid = None
                            pidx = None
                            try:
                                if seat_rec and len(seat_rec) >= 2:
                                    puid = int(seat_rec[0])
                                    pidx = int(seat_rec[1])
                            except Exception:
                                sys.exc_clear()
                            if puid and pidx is not None and pidx >= 0:
                                ao_e, ao_c = self._FindActionObjectByUID(puid)
                                if ao_e is not None and ao_c is not None:
                                    exit_pose = self._GetRemoteApproachWorldPose(ao_e, ao_c, pidx)
                                    if exit_pose is not None:
                                        ex, ey, ez, eyaw = exit_pose
                                        try:
                                            pcx = avatar.GetComponent('position')
                                            if pcx is not None:
                                                pcx.position = (ex, ey, ez)
                                                pcx.rotation = geo2.QuaternionRotationSetYawPitchRoll(eyaw, 0.0, 0.0)
                                            self._targets[charID] = (float(ex), float(ey), float(ez))
                                            self._targetsYaw[charID] = float(eyaw)
                                            self._oneShotPosSnap.add(charID)
                                            log.LogInfo('[CQ-P2v] remote stand exit pose (approach) charID=', charID,
                                                        'x=', ex, 'y=', ey, 'z=', ez, 'yaw=', eyaw)
                                        except Exception:
                                            log.LogException('[CQ-P2v] stand exit pose write')
                                            sys.exc_clear()
                            if seat_rec[1] is not None and int(seat_rec[1]) >= 0:
                                # Crucible logs Trinity: cq_couch_stand_up_play — requestIDs
                                # use the same *Play suffix as the morpheme network.
                                st_cands = (
                                    'cq_couch_stand_up_play', 'couch_stand_up_play',
                                    'cq_couch_stand_up', 'couch_stand_up', 'couch_stand',
                                    'stand_up', 'standup', 'get_up', 'getup', 'couchstand',
                                )
                                self._DriveRemoteAnim(charID, st_cands, 'stand', None)
                            self._lastRemoteActMorpheme[charID] = (0, -1)
                            if charID in self._lastRemoteSitSeat:
                                del self._lastRemoteSitSeat[charID]
                            return

                        ao_entity, ao_comp = self._FindActionObjectByUID(uid)
                        if ao_entity is None or ao_comp is None:
                            log.LogError('[CQ-P2f] _ApplyRemoteAction: no scene actionObject for uid=', uid,
                                         '(charID=', charID, ') -- will retry on next notify')
                            return
                        stations = getattr(ao_comp, 'actionStations', None)
                        if stations is None or idx >= len(stations):
                            log.LogError('[CQ-P2f] _ApplyRemoteAction: invalid stationIdx=', idx,
                                         'len(stations)=', (len(stations) if stations else None),
                                         'charID=', charID)
                            return
                        pose = self._GetRemoteSitWorldPose(ao_entity, ao_comp, idx)
                        if pose is None:
                            log.LogError('[CQ-P2l] _GetRemoteSitWorldPose failed charID=', charID,
                                         'uid=', uid, 'idx=', idx)
                            return
                        wx, wy, wz, wyaw = pose
                        try:
                            pc0 = avatar.GetComponent('position')
                            if pc0 is not None:
                                pc0.position = (wx, wy, wz)
                                pc0.rotation = geo2.QuaternionRotationSetYawPitchRoll(wyaw, 0.0, 0.0)
                            self._targets[charID] = (float(wx), float(wy), float(wz))
                            self._targetsYaw[charID] = float(wyaw)
                            self._oneShotPosSnap.add(charID)
                            log.LogInfo('[CQ-P2m] remote sit pose charID=', charID,
                                        'x=', wx, 'y=', wy, 'z=', wz, 'yaw=', wyaw)
                        except Exception:
                            log.LogException('[CQ-P2l] _ApplyRemoteAction sit pose write')
                            sys.exc_clear()
                            return
                        try:
                            b1 = __import__('__builtin__')
                            M1 = getattr(b1, '_EVEMuCQStashV1', None)
                            if M1 is None:
                                M1 = [None, {}, set()]
                                setattr(b1, '_EVEMuCQStashV1', M1)
                            elif len(M1) < 3:
                                M1.append(set())
                            M1[2].add(int(eid))
                        except Exception:
                            sys.exc_clear()
                        self._lastRemoteSitSeat[charID] = (int(uid), int(idx))
                        act_key = (int(uid), int(idx))
                        if self._lastRemoteActMorpheme.get(charID) != act_key:
                            # L/C/R seats use different *Play / morpheme names in Crucible;
                            # idx=2 (right) does not use the same request as center.
                            if int(idx) == 0:
                                per = (
                                    'cq_couch_sit_down_l_play', 'couch_sit_down_l_play', 'couch_sit_l_play',
                                    'cq_sit_l_down', 'cq_couch_sit_l', 'sit_l_down',
                                )
                            elif int(idx) == 1:
                                per = (
                                    'cq_couch_sit_down_play', 'couch_sit_down_play',
                                    'cq_couch_sit_down_c_play', 'couch_sit_c_play', 'couch_sit_c',
                                )
                            else:
                                per = (
                                    'cq_couch_sit_down_r_play', 'couch_sit_down_r_play', 'couch_sit_r_play',
                                    'cq_sit_r_down', 'cq_couch_sit_r', 'sit_r_down', 'couch_sit_r',
                                )
                            rest = (
                                'cq_couch_sit_down_play', 'couch_sit_down_play',
                                'cq_couch_sit_down', 'cq_couch_sit', 'couch_sit_down',
                                'couch_sit', 'couch_sit_l', 'couch_sit_c', 'couch_sit_r',
                                'sit_down', 'sitdown', 'SIT', 'inc_sit', 'Inc_Sit',
                            )
                            sit_cands = per + rest
                            if self._DriveRemoteAnim(charID, sit_cands, 'sit', int(idx)):
                                self._lastRemoteActMorpheme[charID] = act_key
                                log.LogInfo('[CQ-P2q] remote sit morpheme ok charID=', charID, 'act=', act_key)
                            else:
                                log.LogError('[CQ-P2q] remote sit morpheme miss charID=', charID,
                                             ' (see P2g req dump); pose still applied')
                    except Exception as e:
                        log.LogException('[CQ-P2f] _ApplyRemoteAction error charID=' + str(charID) + ': ' + str(e))
                        sys.exc_clear()

                def PlayLocalEmote(self, emote_index):
                    # Local morpheme + server fan-out. Morpheme name must be in
                    # requestIDs; otherwise the clip is a no-op.
                    try:
                        n = len(_em_menu)
                        if emote_index < 0 or emote_index >= n:
                            return
                        name = _em_menu[emote_index][1]
                        ent_client = getattr(self._view, 'entityClient', None)
                        if ent_client is None:
                            return
                        ent = ent_client.GetPlayerEntity()
                        if ent is None:
                            return
                        anim = ent.GetComponent('animation')
                        if anim is None:
                            return
                        ctrl = getattr(anim, 'controller', None)
                        if ctrl is not None and getattr(ctrl, 'requestIDs', None) is not None:
                            if name in ctrl.requestIDs:
                                ctrl.BroadcastRequest(name)
                            else:
                                log.LogInfo('[CQ-P3] emote request not in network name=', name)
                        mon = self._moniker
                        if mon is not None:
                            mon.PlayEmote(emote_index)
                    except Exception as e:
                        log.LogException('[CQ-P3] PlayLocalEmote error: ' + str(e))
                        sys.exc_clear()

                def OnCQAvatarEmote(self, *a, **kw):
                    try:
                        if len(a) < 2:
                            return
                        cid = a[0]
                        idx = int(a[1])
                        if not cid or cid == session.charid:
                            return
                        self._ApplyRemoteEmote(cid, idx)
                    except Exception as e:
                        log.LogException('[CQ-P3] OnCQAvatarEmote error: ' + str(e))
                        sys.exc_clear()

                def _ApplyRemoteEmote(self, charID, emote_index):
                    if emote_index < 0 or emote_index >= len(_em_menu):
                        return
                    name = _em_menu[emote_index][1]
                    self._DriveRemoteAnim(charID, [name], 'emote')

                # Resolve the local player's current action state by asking the
                # action object manager. Returns (uid, stationIdx) where
                # (0, -1) means "not on any AO" (standing).
                def _GetLocalActionState(self):
                    try:
                        ao_svc = sm.GetService('actionObjectClientSvc')
                        if ao_svc is None or getattr(ao_svc, 'manager', None) is None:
                            return (0, -1)
                        manager = ao_svc.manager
                        my_id = session.charid
                        try:
                            using = bool(manager.IsEntityUsingActionObject(my_id))
                        except Exception:
                            using = False
                            sys.exc_clear()
                        if not using:
                            return (0, -1)
                        try:
                            ao_eid = manager.GetAOEntIDUsedByEntity(my_id)
                        except Exception:
                            ao_eid = None
                            sys.exc_clear()
                        if not ao_eid:
                            return (0, -1)
                        ent_client = getattr(self._view, 'entityClient', None)
                        if ent_client is None:
                            return (0, -1)
                        ao_entity = ent_client.FindEntityByID(ao_eid)
                        if ao_entity is None:
                            return (0, -1)
                        ao_comp = ao_entity.GetComponent('actionObject')
                        if ao_comp is None:
                            return (0, -1)
                        ao_data = getattr(ao_comp, 'actionObjectData', None)
                        uid = int(ao_data.UID) if ao_data is not None else 0
                        stations = getattr(ao_comp, 'actionStations', None)
                        if stations is None:
                            return (uid, -1)
                        for i, station in enumerate(stations):
                            occ = getattr(station, 'occupant', None)
                            if occ == my_id:
                                return (uid, i)
                        # On AO but not registered to any station yet (mid-anim).
                        # Treat as "not yet sitting" -- the next poll will pick
                        # up the real seat once the transition completes.
                        return (0, -1)
                    except Exception as e:
                        log.LogException('[CQ-P2f] _GetLocalActionState error: ' + str(e))
                        sys.exc_clear()
                        return (0, -1)

                # ---- Phase 2f: poll local action state and broadcast --------
                # 4 Hz is plenty for sit/stand transitions. We deliberately
                # don't piggyback on _BroadcastTick because once the avatar
                # sits down position broadcasts go silent (no movement) and
                # we still need to ship the eventual stand-up.
                def _ActionPollTick(self):
                    TICK_MS = 250
                    log.LogInfo('[CQ-P2f] action poll tick started')
                    while not self._stop:
                        try:
                            mon = self._moniker
                            if mon is None:
                                break
                            cur = self._GetLocalActionState()
                            if cur != self._lastSentAction:
                                try:
                                    mon.SetActionState(cur[0], cur[1])
                                    self._lastSentAction = cur
                                    log.LogInfo('[CQ-P2f] sent SetActionState aoUID=', cur[0], 'idx=', cur[1])
                                except Exception as re:
                                    log.LogException('[CQ-P2f] SetActionState RPC error: ' + str(re))
                                    sys.exc_clear()
                        except Exception as e:
                            log.LogException('[CQ-P2f] action poll tick error: ' + str(e))
                            sys.exc_clear()
                        blue.pyos.synchro.SleepWallclock(TICK_MS)
                    log.LogInfo('[CQ-P2f] action poll tick exiting')

                # ---- Phase 2b/d: poll local player transform and push to server ----
                # Runs in its own uthread; samples the local player's
                # PositionComponent every TICK_MS, fires UpdateTransform when
                # the avatar moves more than POS_EPSILON or yaws more than
                # YAW_EPSILON. 10 Hz gives the remote interp tick fine-grained
                # samples to ease through.
                def _BroadcastTick(self):
                    TICK_MS = 100
                    POS_EPSILON_SQ = 0.0025  # ~5cm
                    YAW_EPSILON = 0.017  # ~1 degree
                    log.LogInfo('[CQ-P2b] broadcast tick started')
                    while not self._stop:
                        try:
                            mon = self._moniker
                            if mon is None:
                                break
                            ent_client = getattr(self._view, 'entityClient', None)
                            entity = None
                            if ent_client is not None:
                                try:
                                    entity = ent_client.GetPlayerEntity()
                                except Exception:
                                    entity = None
                                    sys.exc_clear()
                            if entity is None:
                                blue.pyos.synchro.SleepWallclock(TICK_MS)
                                continue
                            pos_comp = entity.GetComponent('position')
                            if pos_comp is None:
                                blue.pyos.synchro.SleepWallclock(TICK_MS)
                                continue
                            p = pos_comp.position
                            x = float(p[0])
                            y = float(p[1])
                            z = float(p[2])
                            yaw = 0.0
                            try:
                                rot = pos_comp.rotation
                                if rot is not None:
                                    ypr = geo2.QuaternionRotationGetYawPitchRoll(rot)
                                    yaw = float(ypr[0])
                            except Exception:
                                yaw = 0.0
                                sys.exc_clear()
                            send = False
                            if self._lastSentPos is None or self._lastSentYaw is None:
                                send = True
                            else:
                                lx, ly, lz = self._lastSentPos
                                dx = x - lx
                                dy = y - ly
                                dz = z - lz
                                if (dx * dx + dy * dy + dz * dz) > POS_EPSILON_SQ:
                                    send = True
                                else:
                                    # Shortest-arc yaw delta in [-pi, pi].
                                    dyaw = yaw - self._lastSentYaw
                                    while dyaw > math.pi:
                                        dyaw -= 2.0 * math.pi
                                    while dyaw < -math.pi:
                                        dyaw += 2.0 * math.pi
                                    if abs(dyaw) > YAW_EPSILON:
                                        send = True
                            if send:
                                try:
                                    mon.UpdateTransform(x, y, z, yaw)
                                    self._lastSentPos = (x, y, z)
                                    self._lastSentYaw = yaw
                                except Exception as e:
                                    log.LogException('[CQ-P2b] UpdateTransform RPC error: ' + str(e))
                                    sys.exc_clear()
                        except Exception as e:
                            log.LogException('[CQ-P2b] tick error: ' + str(e))
                            sys.exc_clear()
                        blue.pyos.synchro.SleepWallclock(TICK_MS)
                    log.LogInfo('[CQ-P2b] broadcast tick exiting')

                # ---- Phase 2c/d: receive-side interpolation -----------------
                # Runs at ~30 Hz, easing each remote avatar's PositionComponent
                # (position AND rotation) toward its latest authoritative
                # target. Exponential alpha gives a framerate-independent
                # ~80ms half-life: snappy enough to track running/turning,
                # smooth enough to hide the 100ms network sample boundaries
                # that previously looked like teleports. Position uses linear
                # ease; yaw uses shortest-arc angular ease. We lerp position
                # and yaw with the same alpha so visually they stay coherent
                # (footsteps and torso turn don't drift apart).
                def _InterpolationTick(self):
                    TICK_MS = 33
                    DT = TICK_MS / 1000.0
                    # Slightly slower convergence + rare snap = less jitter; large
                    # errors lerp in instead of 5m snap (which spiked run anim).
                    LERP_RATE = 8.5
                    ALPHA = 1.0 - math.exp(-LERP_RATE * DT)
                    # Only hard-snap on huge teleports (e.g. respawn), not 5m hops.
                    SNAP_DIST_SQ = 400.0  # 20m
                    # Nudge a bit faster when still far, gentler when almost there.
                    FAR_DIST_SQ = 4.0  # 2m
                    NEAR_DIST_SQ = 0.04  # 0.2m
                    MIN_DIST_SQ = 1e-6
                    YAW_MIN = 1e-4  # ~0.006 deg, below this skip rotation write
                    TWO_PI = 2.0 * math.pi
                    log.LogInfo('[CQ-P2c] interp tick started alpha=%.3f' % ALPHA)
                    while not self._stop:
                        try:
                            ent_client = getattr(self._view, 'entityClient', None)
                            if ent_client is not None and self._targets:
                                for cid, target in list(self._targets.items()):
                                    if cid == session.charid:
                                        continue
                                    eid = self._avatars.get(cid)
                                    if eid is None:
                                        continue
                                    entity = ent_client.FindEntityByID(eid)
                                    if entity is None:
                                        continue
                                    pos_comp = entity.GetComponent('position')
                                    if pos_comp is None:
                                        continue
                                    if cid in self._oneShotPosSnap:
                                        self._oneShotPosSnap.discard(cid)
                                        pos_comp.position = (target[0], target[1], target[2])
                                    cur = pos_comp.position
                                    cx = float(cur[0])
                                    cy = float(cur[1])
                                    cz = float(cur[2])
                                    tx = target[0]
                                    ty = target[1]
                                    tz = target[2]
                                    dx = tx - cx
                                    dy = ty - cy
                                    dz = tz - cz
                                    dsq = dx * dx + dy * dy + dz * dz
                                    if dsq >= MIN_DIST_SQ:
                                        if dsq > SNAP_DIST_SQ:
                                            pos_comp.position = (tx, ty, tz)
                                        else:
                                            a = ALPHA
                                            if dsq > FAR_DIST_SQ:
                                                a = min(0.45, a * 1.85)
                                            elif dsq < NEAR_DIST_SQ:
                                                a = a * 0.65
                                            pos_comp.position = (cx + dx * a,
                                                                 cy + dy * a,
                                                                 cz + dz * a)
                                    # Phase 2d: yaw ease toward _targetsYaw[cid].
                                    # Wrap delta into [-pi, pi] so a +179 -> -179
                                    # transition takes the short way (2 deg)
                                    # rather than spinning 358 deg the long way.
                                    target_yaw = self._targetsYaw.get(cid)
                                    if target_yaw is None:
                                        continue
                                    try:
                                        rot = pos_comp.rotation
                                        if rot is None:
                                            continue
                                        ypr = geo2.QuaternionRotationGetYawPitchRoll(rot)
                                        cur_yaw = float(ypr[0])
                                        dyaw = target_yaw - cur_yaw
                                        while dyaw > math.pi:
                                            dyaw -= TWO_PI
                                        while dyaw < -math.pi:
                                            dyaw += TWO_PI
                                        if abs(dyaw) < YAW_MIN:
                                            continue
                                        new_yaw = cur_yaw + dyaw * ALPHA
                                        pos_comp.rotation = geo2.QuaternionRotationSetYawPitchRoll(new_yaw, 0.0, 0.0)
                                    except Exception:
                                        sys.exc_clear()
                        except Exception as e:
                            log.LogException('[CQ-P2c] interp error: ' + str(e))
                            sys.exc_clear()
                        blue.pyos.synchro.SleepWallclock(TICK_MS)
                    log.LogInfo('[CQ-P2c] interp tick exiting')

            self._cqPhase1Helper = _CQPhase1Helper()
            b = __import__('__builtin__')
            k = '_EVEMuCQStashV1'
            M = getattr(b, k, None)
            if M is None:
                M = [None, {}, set()]
                setattr(b, k, M)
            else:
                if len(M) < 3:
                    M.append(set())
            M[0] = self._cqPhase1Helper
            sm.RegisterNotify(self._cqPhase1Helper)
            log.LogInfo('[CQ-P2] registered notify helper')
        else:
            b = __import__('__builtin__')
            M = getattr(b, '_EVEMuCQStashV1', None)
            if M is None:
                M = [None, {}, set()]
                setattr(b, '_EVEMuCQStashV1', M)
            else:
                if len(M) < 3:
                    M.append(set())
            M[0] = self._cqPhase1Helper
        cq = util.Moniker('captainsQuartersSvc', (ws, const.groupWorldSpace))
        cq.SetSessionCheck({'worldspaceid': ws})
        # Pin the moniker on self so the bound proxy is not GC'd 10-25s
        # after this function returns (which would trigger BoundReleased
        # server-side and force a re-bind on every notify-driven call).
        self._cqPhase1Moniker = cq
        self._cqPhase1Helper._moniker = cq
        log.LogInfo('[CQ-P2] calling JoinSharedQuarters(ws=%s)' % ws)
        cq.JoinSharedQuarters()
        log.LogInfo('[CQ-P2] JoinSharedQuarters returned')
        # Phase 2b/c/f: kick off broadcast tick (sender), interp tick
        # (receiver), and action poll tick (discrete sit/stand sender).
        # Idempotent: if we re-enter _GoStation (rare but possible) the
        # helper is reused and we just refresh the moniker; the prior ticks
        # keep running and pick up the new moniker via `self._moniker`.
        if not getattr(self._cqPhase1Helper, '_tickStarted', False):
            self._cqPhase1Helper._tickStarted = True
            try:
                uthread.new(self._cqPhase1Helper._BroadcastTick)
            except Exception as te:
                log.LogException('[CQ-P2b] failed to start broadcast tick: ' + str(te))
                sys.exc_clear()
                self._cqPhase1Helper._tickStarted = False
            try:
                uthread.new(self._cqPhase1Helper._InterpolationTick)
            except Exception as te:
                log.LogException('[CQ-P2c] failed to start interp tick: ' + str(te))
                sys.exc_clear()
            try:
                uthread.new(self._cqPhase1Helper._ActionPollTick)
            except Exception as te:
                log.LogException('[CQ-P2f] failed to start action poll tick: ' + str(te))
                sys.exc_clear()
        try:
            snap = cq.GetSnapshot()
            log.LogInfo('[CQ-P2] snapshot rows=', (len(snap) if snap else 0))
            if snap:
                for row in snap:
                    try:
                        rcid = row[0]
                        rpos = (float(row[1]), float(row[2]), float(row[3]))
                        # Phase 2d: row[9] = yaw (radians). Tolerate older
                        # 9-tuple rows in case the snapshot was produced
                        # against a pre-rotation server build.
                        ryaw = float(row[9]) if len(row) >= 10 else 0.0
                        # Phase 2f: rows 10/11 = (actionObjectUID, stationIdx).
                        # Stash before Spawn so Spawn's _ApplyRemoteAction
                        # call can pick the value up immediately.
                        rapp = int(row[12]) if len(row) >= 13 else 0
                        if len(row) >= 12:
                            ruid = int(row[10])
                            ridx = int(row[11])
                            if ridx >= 0 and ruid:
                                self._cqPhase1Helper._remoteActions[rcid] = (ruid, ridx)
                        self._cqPhase1Helper.Spawn(rcid, rpos, ryaw, rapp)
                    except Exception as inner:
                        log.LogException('[CQ-P2] snapshot row spawn error: ' + str(inner))
                        sys.exc_clear()
        except Exception as se:
            log.LogException('[CQ-P2] snapshot error: ' + str(se))
            sys.exc_clear()
    except Exception as e:
        log.LogException('[CQ-P2] _GoStation hook error: ' + str(e))
        sys.exc_clear()


# ---------------------------------------------------------------------------
# Patch B: viewstate.CQView::CQView.UnloadView
# ---------------------------------------------------------------------------
def UnloadView_patched(self):
    try:
        helper = getattr(self, '_cqPhase1Helper', None)
        if helper is not None:
            # Stop the broadcast tick first; the next sleep cycle will exit
            # cleanly. Clearing _moniker is belt-and-suspenders for the case
            # where _stop is still being polled when we drop the reference.
            try:
                helper._stop = True
                helper._moniker = None
            except Exception as e:
                log.LogException('[CQ-P2b] tick shutdown error: ' + str(e))
                sys.exc_clear()
            try:
                helper.DespawnAll()
            except Exception as e:
                log.LogException('[CQ-P2] DespawnAll error: ' + str(e))
                sys.exc_clear()
            try:
                ws = session.worldspaceid
                if ws:
                    cq = getattr(self, '_cqPhase1Moniker', None)
                    if cq is None:
                        cq = util.Moniker('captainsQuartersSvc', (ws, const.groupWorldSpace))
                        cq.SetSessionCheck({'worldspaceid': ws})
                    cq.LeaveSharedQuarters()
                    log.LogInfo('[CQ-P2] LeaveSharedQuarters called for ws=', ws)
            except Exception as e:
                log.LogException('[CQ-P2] leave error: ' + str(e))
                sys.exc_clear()
            try:
                sm.UnregisterNotify(helper)
                log.LogInfo('[CQ-P2] unregistered notify helper')
            except Exception as e:
                log.LogException('[CQ-P2] unregister error: ' + str(e))
                sys.exc_clear()
            self._cqPhase1Helper = None
            self._cqPhase1Moniker = None
            try:
                b = __import__('__builtin__')
                M = getattr(b, '_EVEMuCQStashV1', None)
                if M is not None:
                    M[0] = None
                    try:
                        M[1].clear()
                    except Exception:
                        sys.exc_clear()
                    try:
                        if len(M) > 2:
                            M[2].clear()
                    except Exception:
                        sys.exc_clear()
            except Exception:
                sys.exc_clear()
    except Exception as e:
        log.LogException('[CQ-P2] UnloadView pre-hook error: ' + str(e))
        sys.exc_clear()
    if self.stationID is not None:
        self.cameraClient.ExitWorldspace()
        self.worldSpaceClient.UnloadWorldSpaceInstance(self.stationID)
        self.entityClient.UnloadEntityScene(self.stationID)
        viewstate.StationView.UnloadView(self)


# ---------------------------------------------------------------------------
# Patch C: cef.ClientOnlyPlayerSpawner::ClientOnlyPlayerSpawner.__init__
#
# Replace `cfg.eveowners.Get(session.charid).Type().id` with a
# `self.charID`-based lookup. For the local player this is identical
# behaviour (self.charID == session.charid). For remote pilots it now
# resolves the correct player typeID instead of always using the local
# pilot's typeID.
# ---------------------------------------------------------------------------
def ClientOnlyPlayerSpawner_init_patched(self, sceneID, charID, position, rotation, paperdolldna, pubInfo):
    cef.BaseSpawner.__init__(self, sceneID)
    self.charID = charID
    self.playerTypeID = cfg.eveowners.Get(self.charID).Type().id
    self.playerRecipeID = const.cef.PLAYER_RECIPES[self.playerTypeID]
    self.position = position
    self.rotation = rotation
    self.paperdolldna = paperdolldna
    self.pubInfo = pubInfo


# ---------------------------------------------------------------------------
# Patch D: cef.ClientOnlyPlayerSpawner::ClientOnlyPlayerSpawner.GetRecipe
#
# Honour `self.charID` and `self.pubInfo.gender` for the info-component
# overrides instead of `session.charid` / `session.genderID`. Local player
# behaviour is unchanged because session matches self for the local pilot;
# remote pilots now get their real name and gender on the entity.
# ---------------------------------------------------------------------------
def ClientOnlyPlayerSpawner_GetRecipe_patched(self, entityRecipeSvc):
    overrides = {}
    overrides = self._OverrideRecipePosition(overrides, self.GetPosition(), self.GetRotation())
    overrides[const.cef.INFO_COMPONENT_ID] = {'name': cfg.eveowners.Get(self.charID).ownerName,
     'gender': self.pubInfo.gender}
    overrides[const.cef.PAPER_DOLL_COMPONENT_ID] = {'gender': self.pubInfo.gender,
     'dna': self.paperdolldna,
     'typeID': self.playerTypeID}
    recipe = entityRecipeSvc.GetRecipe(self.playerRecipeID, overrides)
    if recipe is None:
        raise RuntimeError('Player Recipe is missing for typeID: %d with recipeID: %d' % (self.playerTypeID, self.playerRecipeID))
    return recipe


# ---------------------------------------------------------------------------
# Patch F: CharControl.GetMenu (base) + EveCharControl.GetMenu — Emotes on self,
# Agent -> Start conversation for remotes / CQ custom agents (mission char id).
# Crucible CQ often uses the base CharControl instance, not EveCharControl, so
# both must be patched or right-clicks never reach agentMgr (see lbw diff).
# ---------------------------------------------------------------------------
def _cq_charcontrol_getmenu_impl(self):
    # Emote list must be local: host module has no CQ_EMOTE_MENU global.
    EM = (('Wave', 'emote_wave'),
     ('Bow', 'emote_bow'),
     ('Applause', 'emote_clap'),
     ('Dance', 'emote_dance'),
     ('Point', 'emote_point'),
     ('Salute', 'emote_salute'),
     ('Fidget', 'fidget_01'),
     ('Stretch', 'fidget_02'))
    uicore = __import__('uicore')
    x = uicore.ScaleDpi(uicore.uilib.x)
    y = uicore.ScaleDpi(uicore.uilib.y)
    self.contextMenuClient = sm.GetService('contextMenuClient')
    entityID = self._PickObject(x, y)
    m = None
    if entityID:
        m = self.contextMenuClient.GetMenuForEntityID(entityID)
    else:
        altPickObject = self._PickHangarScene(x, y)
        if altPickObject and hasattr(altPickObject, 'name') and altPickObject.name == str(util.GetActiveShip()):
            m = self.GetShipMenu()
    if m is None:
        m = []

    def _emote_cb(i):
        h = None
        try:
            b = __import__('__builtin__')
            M = getattr(b, '_EVEMuCQStashV1', None)
            if M is not None:
                h = M[0]
        except Exception:
            sys.exc_clear()
        if h is not None:
            h.PlayLocalEmote(i)
    try:
        if entityID and session.worldspaceid:
            ec = sm.GetService('entityClient')
            h_ok = False
            M2 = None
            try:
                b = __import__('__builtin__')
                M2 = getattr(b, '_EVEMuCQStashV1', None)
                h_ok = M2 is not None and M2[0] is not None
            except Exception:
                sys.exc_clear()
            # Crucible: IsClientSideOnly can be false in shared CQ; our _GoStation helper is authoritative.
            ws_client_only = False
            try:
                ws_client_only = ec.IsClientSideOnly(session.worldspaceid)
            except Exception:
                sys.exc_clear()
            if h_ok or ws_client_only:
                pl = ec.GetPlayerEntity()
                if pl is not None and pl.entityID == entityID and EM and h_ok:
                    m.append(None)
                    sub = []
                    for i in range(len(EM)):
                        sub.append((EM[i][0], _emote_cb, (i,)))
                    m.append(('Emotes', sub))
                    m.append(None)

                    def _cq_reg():
                        try:
                            mon = getattr(M2[0], '_moniker', None) if h_ok and M2[0] else None
                            if mon is None:
                                return
                            n = mon.CQDebugRegisterAgentHere(session.charid, 'rclick')
                            if n is not None:
                                if len(M2) < 4:
                                    M2.append(None)
                                M2[3] = n
                                log.LogInfo('[CQ] CQDebugRegisterAgentHere -> instance', n)
                        except Exception as e:
                            log.LogException('[CQ] CQDebugRegisterAgentHere: ' + str(e))
                            sys.exc_clear()

                    def _cq_move_last():
                        try:
                            mon = getattr(M2[0], '_moniker', None) if h_ok and M2[0] else None
                            if mon is None or len(M2) < 4 or M2[3] is None:
                                log.LogWarn('[CQ] CQDebugUpdateAgentHere: no last instance (register first)')
                                return
                            mon.CQDebugUpdateAgentHere(int(M2[3]))
                        except Exception as e:
                            log.LogException('[CQ] CQDebugUpdateAgentHere: ' + str(e))
                            sys.exc_clear()

                    m.append(('EVEmu CQ (dev role)', (('Register static agent (my paper doll)', _cq_reg, ()),
                        ('Reposition last static agent to me', _cq_move_last, ()))))
                if pl is not None and entityID and entityID != pl.entityID and h_ok:
                    rem_cid = None
                    try:
                        rem_cid = _cq_resolve_remote_char_from_entity_pick(M2[0], entityID)
                    except Exception:
                        sys.exc_clear()
                    if rem_cid is not None:
                        def _agent_talk_cb(rc=rem_cid):
                            try:
                                mon = util.Moniker('agentMgr', (rc,))
                                mon.DoAction(None)
                            except Exception as e3:
                                log.LogException('[CQ] agentMgr DoAction (Start conversation): ' + str(e3))
                                sys.exc_clear()
                        m.append(None)
                        m.append(('Agent', (('Start conversation', _agent_talk_cb, ()),)))
    except Exception:
        log.LogException('[CQ-P3] CharControl GetMenu emote append error')
        sys.exc_clear()
    return m


def EveCharControl_GetMenu_patched(self):
    return _cq_charcontrol_getmenu_impl(self)


def CharControl_GetMenu_patched(self):
    return _cq_charcontrol_getmenu_impl(self)


# ---------------------------------------------------------------------------
# Patch E: animation.BipedAnimationController::BipedAnimationController.UpdateMovement
#
# Original code feeds the morpheme `Speed` / `Moving` parameters off
# self.entityRef.movement.physics.velocity. That works for AI-driven NPCs
# whose physics body is integrated by the engine, but our script-driven
# remote CQ avatars are moved purely by writing position.position from
# the LiveUpdate interp tick -- they have no character controller and no
# physics simulation, so physics.velocity stays (0, 0, 0) forever and the
# remote avatar slides around frozen in idle pose ("gliding").
#
# _UpdateEntityInfo already differentiates positionHistory each frame and
# maintains velocityHistory in m/s. We just average a short trailing
# window of those samples (smooths out per-frame jitter caused by our
# 30 Hz interp tick vs the ~60 Hz frame loop) and use that for Speed /
# Moving instead. Local pilots are unaffected because PlayerAnimationController
# overrides UpdateMovement entirely with its own ProcessNewMovement path.
# TurnAngle is forced to 0.0 -- moveState.GetImmediateRotation() is not
# meaningful for our remote avatars and was never being updated anyway.
#
# Phase 2i/2k: for *remote* CQ avatars only, skip locomotion when the
# ActionObjectManager reports the entity is using a couch, so idle
# locomotion does not override sit morpheme.
# Phase 2r: remotes never register with the AO manager; _EVEMuCQStashV1[2]
# holds entity IDs the server said are seated — skip walk Speed there too.
# NEVER do this for the local pilot (entityID==session.charid): in CQ the
# local entity can still be driven by BipedAnimationController; skipping
# here breaks walk after stand and can confuse AO/interaction.
#
# Phase 2n: for remotes, cap + EMA the displayed walk speed and keep Moving
# off until smoothed speed clears a threshold -- removes jittery run cycles
# and the brief "half run" after position snaps/teleports.
# ---------------------------------------------------------------------------
def BipedAnimationController_UpdateMovement_patched(self):
    EMA_A = 0.22
    MAX_SPD = 3.4
    STOP_D = 0.12
    b = __import__('__builtin__')
    k = '_EVEMuCQStashV1'
    M = getattr(b, k, None)
    if M is None:
        M = [None, {}, set()]
        setattr(b, k, M)
    else:
        if len(M) < 3:
            M.append(set())
    sp = M[1]
    try:
        eref = getattr(self, 'entityRef', None)
        if eref is not None:
            eidA = getattr(eref, 'entityID', None)
            if eidA is not None and eidA != session.charid:
                try:
                    if int(eidA) in M[2]:
                        try:
                            sp.pop(int(eidA), None)
                        except Exception:
                            sys.exc_clear()
                        self.SetControlParameter('Speed', 0.0)
                        self.SetControlParameter('Moving', 0)
                        self.SetControlParameter('TurnAngle', 0.0)
                        return
                except Exception:
                    sys.exc_clear()
                ao = sm.GetService('actionObjectClientSvc')
                if ao is not None and getattr(ao, 'manager', None) is not None:
                    if ao.manager.IsEntityUsingActionObject(eidA):
                        try:
                            sp.pop(int(eidA), None)
                        except Exception:
                            sys.exc_clear()
                        self.SetControlParameter('Speed', 0.0)
                        self.SetControlParameter('Moving', 0)
                        self.SetControlParameter('TurnAngle', 0.0)
                        return
    except Exception:
        sys.exc_clear()
    try:
        history = getattr(self, 'velocityHistory', None)
        n = 0
        if history is not None:
            n = len(history)
        if n > 0:
            k = n
            if k > 8:
                k = 8
            sx = 0.0
            sz = 0.0
            i = 0
            while i < k:
                v = history[i]
                sx += v[0]
                sz += v[2]
                i += 1
            sx /= k
            sz /= k
            speed = math.sqrt(sx * sx + sz * sz)
        else:
            speed = 0.0
    except Exception:
        speed = 0.0
    eid2 = None
    try:
        eref2 = getattr(self, 'entityRef', None)
        if eref2 is not None:
            eid2 = getattr(eref2, 'entityID', None)
    except Exception:
        sys.exc_clear()
    if eid2 is not None and eid2 != session.charid:
        try:
            if speed > MAX_SPD:
                speed = MAX_SPD
            ema_key = int(eid2)
            prev = sp.get(ema_key, 0.0)
            s = EMA_A * speed + (1.0 - EMA_A) * prev
            if speed < 0.08:
                s = (1.0 - STOP_D) * s
            sp[ema_key] = s
            speed = s
        except Exception:
            sys.exc_clear()
    self.SetControlParameter('Speed', speed)
    if speed > 0.2:
        self.SetControlParameter('Moving', 1)
    else:
        self.SetControlParameter('Moving', 0)
    self.SetControlParameter('TurnAngle', 0.0)
