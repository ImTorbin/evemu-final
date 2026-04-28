-- victimAllianceID = -1 triggered KeyError RecordNotFound -1 in Crucible CombatLog_CopyText (EveOwners lookup).
UPDATE chrKillTable SET victimAllianceID = 0 WHERE victimAllianceID < 0;
