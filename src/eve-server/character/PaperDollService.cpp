/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:     caytchen
*/

#include "eve-server.h"


#include "character/PaperDollService.h"
#include "ServiceDB.h"

namespace {
PyResult NewPaperDollDataKeyVal(PaperDollDB& db, uint32 reqCharacterID, uint32 excludeDonorCharID) {
    ServiceDB::EnsureRandomMissionAgentPaperDollInDb(reqCharacterID);
    const uint32 src = db.ResolvePaperDollSourceCharID(reqCharacterID, excludeDonorCharID);
    if (src != reqCharacterID) {
        sLog.Cyan("PaperDoll", "[PaperDoll] doll bundle req=%u -> dollSource=%u (template/donor)", reqCharacterID, src);
    }
    PyDict* args = new PyDict;
    args->SetItemString("colors",     db.GetPaperDollAvatarColors(src));
    args->SetItemString("modifiers",  db.GetPaperDollAvatarModifiers(src));
    args->SetItemString("appearance", db.GetPaperDollAvatar(src));
    args->SetItemString("sculpts",    db.GetPaperDollAvatarSculpts(src));
    return new PyObject("util.KeyVal", args);
}
} // namespace

PaperDollService::PaperDollService() :
    Service("paperDollServer", eAccessLevel_Character)
{
    this->Add("GetPaperDollData", &PaperDollService::GetPaperDollData);
    this->Add("GetMyPaperDollData", &PaperDollService::GetMyPaperDollData);
    this->Add("GetPaperDollDataFor", &PaperDollService::GetPaperDollDataFor);
    this->Add("ConvertAndSavePaperDoll", &PaperDollService::ConvertAndSavePaperDoll);
    this->Add("GetPaperDollPortraitDataFor", &PaperDollService::GetPaperDollPortraitDataFor);
    this->Add("UpdateExistingCharacterFull", &PaperDollService::UpdateExistingCharacterFull);
    this->Add("UpdateExistingCharacterLimited", &PaperDollService::UpdateExistingCharacterLimited);
}

//17:35:32 L PaperDollService::Handle_GetPaperDollData(): size=1
PyResult PaperDollService::GetPaperDollData(PyCallArgs &call, PyInt* characterID) {
    call.Dump(PLAYER__CALL_DUMP);
    if (characterID == nullptr) {
        sLog.Error("PaperDoll", "[PaperDoll] GetPaperDollData: null characterID from char=%u",
            call.client != nullptr ? call.client->GetCharacterID() : 0u);
        return PyStatic.NewNone();
    }
    const uint32 viewerID = (call.client != nullptr) ? call.client->GetCharacterID() : 0u;
    return NewPaperDollDataKeyVal(m_db, static_cast<uint32>(characterID->value()), viewerID);
}

PyResult PaperDollService::ConvertAndSavePaperDoll(PyCallArgs &call) {
    call.Dump(PLAYER__CALL_DUMP);
    return nullptr;
}

PyResult PaperDollService::UpdateExistingCharacterFull(PyCallArgs &call, PyInt* characterID, PyRep* dollInfo, PyRep* portraitInfo, PyBool* dollExists) {
    call.Dump(PLAYER__CALL_DUMP);
    /*
        sm.RemoteSvc('paperDollServer').UpdateExistingCharacterFull(charID, dollInfo, portraitInfo, dollExists)
        */
    return nullptr;
}

PyResult PaperDollService::UpdateExistingCharacterLimited(PyCallArgs &call, PyInt* characterID, PyRep* dollData, PyRep* portraitInfo, PyBool* dollExists) {
    call.Dump(PLAYER__CALL_DUMP);
    /*
        sm.RemoteSvc('paperDollServer').UpdateExistingCharacterLimited(charID, dollData, portraitInfo, dollExists)
        */
    /*)
00:46:38 [SvcCall] Service photoUploadSvc::Upload()
00:46:38 W       ImageServer:  ReportNewImage() called.
00:46:38 M    PhotoUploadSvc: Received image from account 3, size: 57621
00:46:39 [SvcCall] Service paperDollServer::UpdateExistingCharacterLimited()
*/
    return nullptr;
}

PyResult PaperDollService::GetPaperDollPortraitDataFor(PyCallArgs &call, PyInt* characterID) {
    //    data = sm.RemoteSvc('paperDollServer').GetPaperDollPortraitDataFor(charID)
    /*
            portraitData = sm.GetService('cc').GetPortraitData(charID)
            if portraitData is not None:
                self.lightingID = portraitData.lightID
                self.lightColorID = portraitData.lightColorID
                self.lightIntensity = portraitData.lightIntensity
                path = self.GetBackgroundPathFromID(portraitData.backgroundID)
                if path in ccConst.backgroundOptions:
                    self.backdropPath = path
                self.poseID = portraitData.portraitPoseNumber
                self.cameraPos = (portraitData.cameraX, portraitData.cameraY, portraitData.cameraZ)
                self.cameraPoi = (portraitData.cameraPoiX, portraitData.cameraPoiY, portraitData.cameraPoiZ)
                self.cameraFov = portraitData.cameraFieldOfView
                params = self.GetControlParametersFromPoseData(portraitData, fromDB=True).values()
                self.characterSvc.SetControlParametersFromList(params, charID)
    */
    const uint32 reqID = static_cast<uint32>(characterID->value());
    const uint32 viewerID = (call.client != nullptr) ? call.client->GetCharacterID() : 0u;
    ServiceDB::EnsureRandomMissionAgentPaperDollInDb(reqID);
    const uint32 charID = m_db.ResolvePaperDollSourceCharID(reqID, viewerID);
    return m_db.GetPaperDollPortraitData(charID);
}

PyResult PaperDollService::GetMyPaperDollData(PyCallArgs &call, PyInt* characterID)
{
    call.Dump(PLAYER__CALL_DUMP);

	PyDict* args = new PyDict;

	args->SetItemString( "colors", m_db.GetPaperDollAvatarColors(call.client->GetCharacterID()) );
	args->SetItemString( "modifiers", m_db.GetPaperDollAvatarModifiers(call.client->GetCharacterID()) );
	args->SetItemString( "appearance", m_db.GetPaperDollAvatar(call.client->GetCharacterID()) );
	args->SetItemString( "sculpts", m_db.GetPaperDollAvatarSculpts(call.client->GetCharacterID()) );

    return new PyObject("util.KeyVal", args);
}

// Same shape as GetMyPaperDollData but keyed on the requested characterID.
// Used by the modded shared-CQ client to materialize remote avatars in one
// RPC instead of N legacy GetPaperDollData / GetPaperDollPortraitDataFor
// round trips per remote occupant.
PyResult PaperDollService::GetPaperDollDataFor(PyCallArgs &call, PyInt* characterID)
{
    call.Dump(PLAYER__CALL_DUMP);

    if (characterID == nullptr) {
        sLog.Error("PaperDoll", "[PaperDoll] GetPaperDollDataFor: null characterID arg from char=%u",
            call.client != nullptr ? call.client->GetCharacterID() : 0u);
        return PyStatic.NewNone();
    }

    const uint32 reqID = static_cast<uint32>(characterID->value());
    const uint32 viewerID = (call.client != nullptr) ? call.client->GetCharacterID() : 0u;
    return NewPaperDollDataKeyVal(m_db, reqID, viewerID);
}
