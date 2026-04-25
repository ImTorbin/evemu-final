-- Phase 0 LiveUpdate pipeline ping.
--
-- Purpose: prove the server -> client live-update channel works end-to-end
-- on this build of EVEmu + Crucible client without modifying any client
-- bytecode. The Crucible client (carbon/common/script/net/GPS.py) iterates
-- the `live_updates` field of CryptoHandshakeAck during login and dispatches
-- each entry through `sm.ScatterEvent('OnLiveClientUpdate', theUpdate.code)`,
-- which lands in carbon/client/script/sys/liveUpdateSvc.py:OnLiveClientUpdate.
--
-- The very first thing that handler does is:
--     self.LogInfo('Applying live update - type:', theUpdate.codeType,
--                  'object:', theUpdate.objectID,
--                  'method:', theUpdate.methodName)
-- so a uniquely-named row in this table guarantees a uniquely-identifiable
-- LogInfo line in LogServer on every successful login.
--
-- We then deliberately point the patch at a non-existent class+namespace
-- ('evemu.phase0.db::Marker'), which causes the dispatcher to fall through
-- to "Failed to apply live update - namespace not found." That LogError is
-- expected and harmless: it does NOT raise, so login is unaffected.
--
-- The `code` payload is `marshal.dumps(None)` in Python 2.7, which is a
-- single byte 0x4E ('N'). The client calls `marshal.loads(theUpdate.code)`
-- before the codeType dispatch, so we must supply a valid marshal stream
-- to avoid an exception that would propagate up through ScatterEvent.

-- +migrate Up
DELETE FROM `liveupdates` WHERE updateName='EVEMuPhase0Ping';
INSERT INTO `liveupdates`
    (updateID, updateName, description,
     machoVersionMin, machoVersionMax, buildNumberMin, buildNumberMax,
     methodName, objectID, codeType, code)
VALUES (
    100,
    'EVEMuPhase0Ping',
    'Phase 0 LiveUpdate pipeline ping (resolves to namespace-not-found)',
    1, 330, 1, 500000,
    'pingMethod',
    'evemu.phase0.db::Marker',
    'globalClassMethod',
    0x4E
);

-- +migrate Down
DELETE FROM `liveupdates` WHERE updateName='EVEMuPhase0Ping';
