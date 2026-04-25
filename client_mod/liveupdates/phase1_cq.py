# -*- coding: utf-8 -*-
# Phase 1 client-side LiveUpdate patches for multiplayer Captain's Quarters.
#
# Compiled with Python 2.7 by build_phase1_sql.py and shipped as marshaled
# func_code blobs in the `liveupdates` table. The Crucible client's
# liveUpdateSvc replaces the matching method's func_code with these blobs at
# handshake time.
#
# The two functions below replace these methods on
#   eve/client/script/ui/view/cqView.py :: CQView
#
#   _GoStation_patched   -> CQView._GoStation
#   UnloadView_patched   -> CQView.UnloadView
#
# IMPORTANT: when liveUpdateSvc swaps func_code, the function's func_globals
# stay as cqView.py's module globals. So we may use any name imported there:
#   sys, log, util, viewstate, localization, uthread, uix
# plus the standard EVE-injected globals: session, sm, cfg, settings, const.
# We MUST NOT use any name not already present in that module.
#
# All Phase 1 hooks are wrapped in try/except so any failure here cannot
# break station entry or exit.

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
        log.LogInfo('[CQ-P1] _GoStation hook ws=', ws, 'st=', st)
        if not ws:
            log.LogInfo('[CQ-P1] no worldspaceid set, skipping bind')
            return
        if not getattr(self, '_cqPhase1Helper', None):
            class _CQPhase1Helper(object):
                __notifyevents__ = ['OnCharNowInStation', 'OnCharNoLongerInStation', 'OnCQAvatarMove']
                def OnCharNowInStation(self, *a, **kw):
                    log.LogInfo('[CQ-P1] OnCharNowInStation', a, kw)
                def OnCharNoLongerInStation(self, *a, **kw):
                    log.LogInfo('[CQ-P1] OnCharNoLongerInStation', a, kw)
                def OnCQAvatarMove(self, *a, **kw):
                    log.LogInfo('[CQ-P1] OnCQAvatarMove', a, kw)
            self._cqPhase1Helper = _CQPhase1Helper()
            sm.RegisterNotify(self._cqPhase1Helper)
            log.LogInfo('[CQ-P1] registered notify helper for', self._cqPhase1Helper.__notifyevents__)
        cq = util.Moniker('captainsQuartersSvc', (ws, const.groupWorldSpace))
        cq.SetSessionCheck({'worldspaceid': ws})
        log.LogInfo('[CQ-P1] calling JoinSharedQuarters(ws=%s)' % ws)
        cq.JoinSharedQuarters()
        log.LogInfo('[CQ-P1] JoinSharedQuarters returned')
        try:
            snap = cq.GetSnapshot()
            log.LogInfo('[CQ-P1] snapshot:', snap)
        except Exception as se:
            log.LogException('[CQ-P1] snapshot error: ' + str(se))
            sys.exc_clear()
    except Exception as e:
        log.LogException('[CQ-P1] _GoStation hook error: ' + str(e))
        sys.exc_clear()


def UnloadView_patched(self):
    try:
        helper = getattr(self, '_cqPhase1Helper', None)
        if helper is not None:
            try:
                ws = session.worldspaceid
                if ws:
                    cq = util.Moniker('captainsQuartersSvc', (ws, const.groupWorldSpace))
                    cq.SetSessionCheck({'worldspaceid': ws})
                    cq.LeaveSharedQuarters()
                    log.LogInfo('[CQ-P1] LeaveSharedQuarters called for ws=', ws)
            except Exception as e:
                log.LogException('[CQ-P1] leave error: ' + str(e))
                sys.exc_clear()
            try:
                sm.UnregisterNotify(helper)
                log.LogInfo('[CQ-P1] unregistered notify helper')
            except Exception as e:
                log.LogException('[CQ-P1] unregister error: ' + str(e))
                sys.exc_clear()
            self._cqPhase1Helper = None
    except Exception as e:
        log.LogException('[CQ-P1] UnloadView pre-hook error: ' + str(e))
        sys.exc_clear()
    if self.stationID is not None:
        self.cameraClient.ExitWorldspace()
        self.worldSpaceClient.UnloadWorldSpaceInstance(self.stationID)
        self.entityClient.UnloadEntityScene(self.stationID)
        viewstate.StationView.UnloadView(self)
