
 /**
  * @name AgentBound.cpp
  *   agent specific code
  *    removed from AgentMgrService.cpp
  *
  * @Author:        Allan
  * @date:      26 June 2018
  *
  */


/*
 * # Agent Logging:
 * AGENT__ERROR
 * AGENT__WARNING
 * AGENT__MESSAGE
 * AGENT__DEBUG
 * AGENT__INFO
 * AGENT__TRACE
 * AGENT__DUMP
 * AGENT__RSP_DUMP
 */

#include "eve-server.h"
#include "../../eve-common/EVE_Defines.h"
#include "../../eve-common/EVE_Missions.h"
//#include "../../eve-common/EVE_Skills.h"
#include "../../eve-common/EVE_Standings.h"

#include "StaticDataMgr.h"
#include "station/StationDataMgr.h"
#include "station/StationDB.h"
#include "account/AccountService.h"
#include "corporation/LPService.h"
#include "manufacturing/Blueprint.h"
#include "../../eve-common/EVE_RAM.h"
#include "../../eve-common/EVE_Dungeon.h"
#include "../fleet/FleetService.h"
#include "agents/AgentBound.h"
#include "agents/AgentMgrService.h"
#include "station/Station.h"
#include "services/ServiceManager.h"
#include "../../eve-common/EVE_Agent.h"

namespace {
    /** Crucible client: "{missionName} Objectives" — needs missionName in GetByMessageID kwargs (often missing). */
    constexpr uint32 kClientMsg_MissionNameObjectives = 235549U;
    /** Same client table: static blurb, no tokens; agentfinder ShowInfo never calls GetMissionKeywords before BuildObjectiveHTML. */
    constexpr uint32 kClientMsg_OverviewObjectivesBlurb = 235551U;

    /** Fills 0 system ids from station/solar item ids; avoids client pathfinder KeyError toID=0. */
    void RefillOfferSystemIds(MissionOffer& offer, const ::Agent* agent)
    {
        if (IsStationID(offer.destinationID) and offer.destinationID != 0) {
            if (offer.destinationSystemID == 0) {
                offer.destinationSystemID = sDataMgr.GetStationSystem(offer.destinationID);
                if (offer.destinationSystemID == 0)
                    offer.destinationSystemID = StationDB::GetSolarSystemIDForStation(offer.destinationID);
            }
        } else if (IsSolarSystemID(offer.destinationID) and offer.destinationID != 0) {
            if (offer.destinationSystemID == 0)
                offer.destinationSystemID = offer.destinationID;
        }
        if (IsStationID(offer.originID) and offer.originID != 0 and offer.originSystemID == 0) {
            offer.originSystemID = sDataMgr.GetStationSystem(offer.originID);
            if (offer.originSystemID == 0)
                offer.originSystemID = StationDB::GetSolarSystemIDForStation(offer.originID);
        }
        // Crucible client: GetSecurityWarning -> GetPathBetween(..., toID=destinationSystemID); KeyError if toID is 0.
        if (offer.destinationSystemID == 0 and agent != nullptr) {
            uint32 sys = agent->GetSystemID();
            if (sys == 0 and IsStationID(agent->GetStationID()) and agent->GetStationID() != 0) {
                sys = sDataMgr.GetStationSystem(agent->GetStationID());
                if (sys == 0)
                    sys = StationDB::GetSolarSystemIDForStation(agent->GetStationID());
            }
            if (sys != 0) {
                // Distant low-sec finale: pinning dest to the agent's station makes pickup and dropoff the same station.
                const bool skipAgentPin =
                    (offer.range == Agents::Range::DistantLowSecEightToFifteenJumps
                     or offer.range == Agents::Range::WithinThirteenJumpsOfAgent)
                    and (offer.destinationID == 0);
                if (skipAgentPin) {
                    _log(AGENT__WARNING,
                         "RefillOfferSystemIds: range=14 courier still has no destination station; not using agent home (would match pickup). "
                         "Fix GetMissionDestination selection for this agent.");
                } else {
                    _log(AGENT__WARNING, "RefillOfferSystemIds: mission dest system still 0; using agent system %u as fallback (fix mission destination data if this repeats).", sys);
                    offer.destinationSystemID = sys;
                    if (offer.destinationID == 0 and IsStationID(agent->GetStationID()) and agent->GetStationID() != 0)
                        offer.destinationID = agent->GetStationID();
                }
            }
        }
    }

    /** Filler smuggling one-liner for Juro (90000007); avoids missing client messageIDs in convo. */
    const char* JuroConvoLine(uint32 agentID, uint16 missionID) {
        if (agentID != 90000007)
            return nullptr;
        switch (missionID) {
            case 58370: return "This one stays in the neighborhood. Do not complicate a simple file.";
            case 58371: return "Short hop. Same deal as last time: you move it, the invoice moves alone.";
            case 58372: return "Local leg three. I am not your conscience; I am the signature line.";
            case 58373: return "This one leaves polite space. You will cross low security or null; plan the tank, not a speech.";
            default:   return "Clock is ticking. Move the package and keep the manifest boring.";
        }
    }

    void SetAgentSaysForOffer(PyTuple* agentSays, uint32 agentID, const MissionOffer& offer) {
        if (const char* line = JuroConvoLine(agentID, offer.missionID)) {
            agentSays->SetItem(0, new PyString(line));
            agentSays->SetItem(1, new PyInt(offer.characterID));
        } else {
            agentSays->SetItem(0, new PyInt(offer.briefingID));
            agentSays->SetItem(1, new PyInt(offer.characterID));
        }
    }
} // namespace

AgentBound::AgentBound(EVEServiceManager& mgr, AgentMgrService& parent, Agent *agt) :
    EVEBoundObject(mgr, parent),
    m_agent(agt)
{
    this->Add("DoAction", &AgentBound::DoAction);
    this->Add("GetAgentLocationWrap", &AgentBound::GetAgentLocationWrap);
    this->Add("GetInfoServiceDetails", &AgentBound::GetInfoServiceDetails);
    this->Add("GetMissionBriefingInfo", &AgentBound::GetMissionBriefingInfo);
    this->Add("GetMissionObjectiveInfo", &AgentBound::GetMissionObjectiveInfo);
    this->Add("GetMissionKeywords", &AgentBound::GetMissionKeywords);
    this->Add("GetMissionJournalInfo", &AgentBound::GetMissionJournalInfo);
    this->Add("GetDungeonShipRestrictions", &AgentBound::GetDungeonShipRestrictions);
    this->Add("RemoveOfferFromJournal", &AgentBound::RemoveOfferFromJournal);
    this->Add("GetOfferJournalInfo", &AgentBound::GetOfferJournalInfo);
    this->Add("GetEntryPoint", &AgentBound::GetEntryPoint);
    this->Add("GotoLocation", &AgentBound::GotoLocation);
    this->Add("WarpToLocation", &AgentBound::WarpToLocation);
    this->Add("GetMyJournalDetails", &AgentBound::GetMyJournalDetails);
}
PyResult AgentBound::GetAgentLocationWrap(PyCallArgs &call) {
    // this is detailed info on agent's location
    return m_agent->GetLocationWrap();
}

PyResult AgentBound::GetInfoServiceDetails(PyCallArgs& call) {
    // this is agents personal info... level, division, station, etc.
    return m_agent->GetInfoServiceDetails();
}


/**     ***********************************************************************
 * @note   these below are partially coded
 */

PyResult AgentBound::DoAction(PyCallArgs &call, std::optional <PyInt*> actionID) {
    // this is first call when initiating agent convo
    _log(AGENT__DUMP,  "AgentBound::Handle_DoAction() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    // this actually returns a complicated tuple depending on other variables involving this agent and char.
    /*
        agentSays, dialogue, extraInfo = self.__GetConversation(agentDialogueWindow, actionID)

    def __GetConversation(self, wnd, actionID):
       tmp = wnd.sr.agentMoniker.DoAction(actionID)
        ret, wnd.sr.oob = tmp
        agentSays, wnd.sr.dialogue = ret
        firstActionID = wnd.sr.dialogue[0][0]
        firstActionDialogue = wnd.sr.dialogue[0][1]
        wnd.sr.agentSays = self.ProcessMessage(agentSays, wnd.sr.agentID)
        return (wnd.sr.agentSays, wnd.sr.dialogue, wnd.sr.oob)
     *
     */

    Character* pchar = call.client->GetChar().get();
    float charStanding = StandingDB::GetStanding(m_agent->GetID(), pchar->itemID());
    float quality = EvEMath::Agent::EffectiveQuality(m_agent->GetQuality(), pchar->GetSkillLevel(EvESkill::Connections), charStanding);
    float bonus = EvEMath::Agent::GetStandingBonus(charStanding, m_agent->GetFactionID(), pchar->GetSkillLevel(EvESkill::Connections), pchar->GetSkillLevel(EvESkill::Diplomacy), pchar->GetSkillLevel(EvESkill::CriminalConnections));
    float standing = EvEMath::Agent::EffectiveStanding(charStanding, bonus);

    std::string response = "";
    bool missionQuit = false, missionCompleted = false, missionDeclined = false;

    PyTuple* agentSays = new PyTuple(2);
    // dialog is button info
    PyList* dialog = new PyList();

    using namespace Dialog::Button;

    // to set 'admin dialog options' (which i dont know wtf they are yet), i *think* you add a tuple of *something* that is NOT dict or int.
    if (false /*admin options*/) {
        PyTuple* adminButton = new PyTuple(2);
        adminButton->SetItem(0, new PyInt(Admin));
        adminButton->SetItem(1, new PyString("Admin Options"));
        dialog->AddItem(adminButton);
    }

    if (m_agent->CanUseAgent(call.client)) {
        switch (actionID.has_value() ? actionID.value()->value() : 0) {
            case 0: {
                //  if char has current mission with this agent, add this one.
                MissionOffer offer = MissionOffer();
                if (m_agent->HasMission(pchar->itemID(), offer)) {
                    PyTuple* button1 = new PyTuple(2);
                        button1->SetItem(0, new PyInt(ViewMission)); // this are buttonIDs which are unique and sequential to each agent, regardless of chars
                        button1->SetItem(1, new PyInt(ViewMission));
                    dialog->AddItem(button1);
                    if (call.client->IsMissionComplete(offer))  {
                        PyTuple* button2 = new PyTuple(2);
                            button2->SetItem(0, new PyInt(Complete));
                            button2->SetItem(1, new PyInt(Complete));
                        dialog->AddItem(button2);
                    }
                    SetAgentSaysForOffer(agentSays, m_agent->GetID(), offer);
                } else {
                    // dialogue data.  if RequestMission is only option, client auto-responds with DoAction(RequestMission optionID)
                    PyTuple* button2 = new PyTuple(2);
                        button2->SetItem(0, new PyInt(RequestMission));
                        button2->SetItem(1, new PyInt(RequestMission));
                    dialog->AddItem(button2);
                // response as string for custom data.  response as pyint to use client data (using getlocale shit)
                    // default initial agent response based on agent location, level, bloodline, quality, and char/agent standings
                    //  this will be modeled after UO speech data, in tiers and levels.
                    // if RequestMission is only option, this is ignored.  see note under 'dialog data'
                    response = "Why the fuck am I looking at you again, ";
                    response += call.client->GetName();
                    response += "?";
                    agentSays->SetItem(0, new PyString(response));  //msgInfo  -- if tuple[0].string then return msgInfo
                    agentSays->SetItem(1, PyStatic.NewNone());      // ContentID  -- PyNone used when msgInfo is string (mostly for initial greetings)
                }

                // if agent does location, add this one...
                if (m_agent->IsLocator()) {
                    PyTuple* button3 = new PyTuple(2);
                        button3->SetItem(0, new PyInt(LocateCharacter));
                        button3->SetItem(1, new PyInt(LocateCharacter));
                    dialog->AddItem(button3);
                }

                // if agent does research, add this one...
                if (m_agent->IsResearch()) {
                    PyTuple* button4 = new PyTuple(2);
                        button4->SetItem(0, new PyInt(StartResearch));
                        button4->SetItem(1, new PyInt(StartResearch));
                    dialog->AddItem(button4);
                }
            } break;
            case RequestMission: {  //2
                MissionOffer offer = MissionOffer();
                m_agent->MakeOffer(pchar->itemID(), offer);
                m_agent->SendMissionUpdate(call.client, "offered");

                //  this one will get complicated and is based on agent/char interaction
                //   detail in /eve/client/script/ui/station/agents/agents.py

                /*  agentSays is a tuple of msgData and contentID
                *      msgData can be single integer of briefingID, a string literal, or a tuple as defined above.
                *   contentID is used for specific char's mission keywords.  we're not using it like there here....
                */

                SetAgentSaysForOffer(agentSays, m_agent->GetID(), offer);

                // dialog can also contain mission data.
                //   set a dialog tuple[1] to dict and fill with MissionBriefingInfo
                PyTuple* button1 = new PyTuple(2);
                    button1->SetItem(0, new PyInt(Accept));
                    button1->SetItem(1, new PyInt(Accept));
                dialog->AddItem(button1);
                PyTuple* button2 = new PyTuple(2);
                    button2->SetItem(0, new PyInt(Decline));
                    button2->SetItem(1, new PyInt(Decline));
                dialog->AddItem(button2);
                PyTuple* button3 = new PyTuple(2);
                    button3->SetItem(0, new PyInt(Defer));
                    button3->SetItem(1, new PyInt(Defer));
                dialog->AddItem(button3);
            } break;
            case ViewMission: { //1
                MissionOffer offer = MissionOffer();
                m_agent->GetOffer(pchar->itemID(), offer);
                SetAgentSaysForOffer(agentSays, m_agent->GetID(), offer);
                if (offer.stateID < Mission::State::Accepted) {
                    PyTuple* button1 = new PyTuple(2);
                        button1->SetItem(0, new PyInt(Accept));
                        button1->SetItem(1, new PyInt(Accept));
                    dialog->AddItem(button1);
                    PyTuple* button2 = new PyTuple(2);
                        button2->SetItem(0, new PyInt(Decline));
                        button2->SetItem(1, new PyInt(Decline));
                    dialog->AddItem(button2);
                    PyTuple* button3 = new PyTuple(2);
                        button3->SetItem(0, new PyInt(Defer));
                        button3->SetItem(1, new PyInt(Defer));
                    dialog->AddItem(button3);
                } else if (offer.stateID == Mission::State::Accepted) {
                    PyTuple* button1 = new PyTuple(2);
                        button1->SetItem(0, new PyInt(Quit));
                        button1->SetItem(1, new PyInt(Quit));
                    dialog->AddItem(button1);
                    if (call.client->IsMissionComplete(offer))  {
                        PyTuple* button2 = new PyTuple(2);
                            button2->SetItem(0, new PyInt(Complete));
                            button2->SetItem(1, new PyInt(Complete));
                        dialog->AddItem(button2);
                    }
                }
            } break;
            case Accept:            //3
            case AcceptRemotely: {  //5
                MissionOffer offer = MissionOffer();
                m_agent->GetOffer(pchar->itemID(), offer);
                offer.stateID = Mission::State::Accepted;
                offer.dateAccepted = GetFileTimeNow();
                offer.expiryTime = GetFileTimeNow() + (30 * m_agent->GetLevel() * EvE::Time::Minute);  // 30m per agent level  ?  test this.
                // Courier/trade: give the package at accept. Mining: objective ore must be mined by the player — do not spawn it.
                if (offer.courierTypeID && offer.typeID != Mission::Type::Mining) {
                    sItemFactory.SetUsingClient(call.client);
                    ItemData data(offer.courierTypeID, pchar->itemID(), locTemp, flagNone, offer.courierAmount);
                    InventoryItemRef iRef = sItemFactory.SpawnItem(data);
                    iRef->Move(offer.originID, flagHangar, true);
                    sItemFactory.UnsetUsingClient();
                }
                if (offer.typeID == Mission::Type::Encounter && offer.dungeonLocationID != 0)
                    _log(AGENT__INFO,
                         "Encounter offer %u: dungeon template %u (deadspace instancing via DungeonMgr::MakeDungeon not yet tied to missions).",
                         offer.missionID, offer.dungeonLocationID);
                m_agent->UpdateOffer(pchar->itemID(), offer);
                m_agent->SendMissionUpdate(call.client, "offer_accepted");
                agentSays->SetItem(0, new PyInt(m_agent->GetAcceptRsp(pchar->itemID())));
                agentSays->SetItem(1, new PyInt(pchar->itemID()));
            } break;
            case Complete:              //6
            case CompleteRemotely: {    //7
                MissionOffer offer = MissionOffer();
                m_agent->GetOffer(pchar->itemID(), offer);
                if (!call.client->IsMissionComplete(offer)) {
                    call.client->SendErrorMsg("Mission objectives are not complete.");
                    agentSays->SetItem(0, PyStatic.NewNone());
                    agentSays->SetItem(1, PyStatic.NewNone());
                    break;
                }
                offer.stateID = Mission::State::Completed;
                offer.dateCompleted = GetFileTimeNow();
                m_agent->UpdateOffer(pchar->itemID(), offer);
                m_agent->SendMissionUpdate(call.client, "completed");
                agentSays->SetItem(0, new PyInt(m_agent->GetCompleteRsp(pchar->itemID())));
                agentSays->SetItem(1, new PyInt(pchar->itemID()));
                if (offer.courierTypeID) {
                    call.client->RemoveMissionItem(offer.courierTypeID, offer.courierAmount);
                }
                if (offer.rewardItemID) {
                    sItemFactory.SetUsingClient(call.client);
                    ItemData data(offer.rewardItemID, pchar->itemID(), locTemp, flagNone, offer.rewardItemQty);
                    InventoryItemRef iRef = sItemFactory.SpawnItem(data);
                    iRef->Move(m_agent->GetStationID(), flagHangar, true);
                    sItemFactory.UnsetUsingClient();
                }
                if (offer.rewardExtraItemID) {
                    sItemFactory.SetUsingClient(call.client);
                    ItemData bpItem(offer.rewardExtraItemID, pchar->itemID(), locTemp, flagNone, 1);
                    EvERam::bpData bpdata = EvERam::bpData();
                    bpdata.copy = true;
                    bpdata.mLevel = 0;
                    bpdata.pLevel = 0;
                    bpdata.runs = 10;
                    BlueprintRef bpRef = Blueprint::Spawn(bpItem, bpdata);
                    if (bpRef.get() != nullptr)
                        bpRef->Move(m_agent->GetStationID(), flagHangar, true);
                    sItemFactory.UnsetUsingClient();
                }
                if (offer.rewardISK > 0) {
                    std::vector<Client*> fleetInSys;
                    if (call.client->InFleet())
                        sFltSvc.GetFleetClientsInSystem(call.client, fleetInSys);
                    if (fleetInSys.size() > 1) {
                        const int64_t total = offer.rewardISK;
                        const size_t n = fleetInSys.size();
                        const int64_t each = total / (int64_t)n;
                        const int64_t rem = total % (int64_t)n;
                        for (size_t i = 0; i < n; ++i) {
                            Client* fc = fleetInSys[i];
                            if (fc == nullptr)
                                continue;
                            const int64_t pay = each + (i < (size_t)rem ? 1 : 0);
                            AccountService::TransferFunds(m_agent->GetID(), fc->GetCharacterID(), pay, "Mission Reward", Journal::EntryType::AgentMissionReward, m_agent->GetID());
                        }
                    } else {
                        AccountService::TransferFunds(m_agent->GetID(), pchar->itemID(), offer.rewardISK, "Mission Reward", Journal::EntryType::AgentMissionReward, m_agent->GetID());
                    }
                }
                bool paidTimeBonus = false;
                if (offer.bonusISK > 0 and offer.bonusTime > 0) {
                    const int64_t deadline = offer.dateAccepted + (int64_t)offer.bonusTime * (int64_t)EvE::Time::Minute;
                    if (GetFileTimeNow() <= deadline) {
                        AccountService::TransferFunds(m_agent->GetID(), pchar->itemID(), offer.bonusISK, "Mission Bonus Reward", Journal::EntryType::AgentMissionTimeBonusReward, m_agent->GetID());
                        paidTimeBonus = true;
                    }
                }
                if (paidTimeBonus)
                    m_agent->UpdateStandings(call.client, Standings::MissionBonus, offer.important);
                if (offer.rewardLP > 0) {
                    if (call.client->InFleet()) {
                        std::vector<Client*> fleetInSys;
                        sFltSvc.GetFleetClientsInSystem(call.client, fleetInSys);
                        if (fleetInSys.size() > 1) {
                            const int n = (int)fleetInSys.size();
                            const int base = offer.rewardLP / n;
                            const int rem = offer.rewardLP % n;
                            for (int i = 0; i < n; ++i) {
                                Client* fc = fleetInSys[static_cast<size_t>(i)];
                                if (fc == nullptr)
                                    continue;
                                const int amt = base + (i < rem ? 1 : 0);
                                LPService::AddLP(fc->GetCharacterID(), m_agent->GetCorpID(), amt);
                            }
                        } else {
                            LPService::AddLP(pchar->itemID(), m_agent->GetCorpID(), offer.rewardLP);
                        }
                    } else {
                        LPService::AddLP(pchar->itemID(), m_agent->GetCorpID(), offer.rewardLP);
                    }
                }
                m_agent->UpdateStandings(call.client, Standings::MissionCompleted, offer.important);
                m_agent->RemoveOffer(pchar->itemID());
            } break;
            case Defer: {   //10
                // extend expiry time and close
                MissionOffer offer = MissionOffer();
                if (m_agent->HasMission(pchar->itemID(), offer)) {
                    offer.stateID = Mission::State::Allocated; //Defered
                    offer.expiryTime += EvE::Time::Day;
                    m_agent->UpdateOffer(pchar->itemID(), offer);
                    m_agent->SendMissionUpdate(call.client, "prolong");
                    agentSays->SetItem(0, new PyString("I can give you 24 hours to think about it."));    //msgInfo  -- if tuple[0].string then return msgInfo
                    agentSays->SetItem(1, PyStatic.NewNone());    // ContentID  -- PyNone used when msgInfo is string to return without processing
                }
            } break;
            case Decline: { //9
                missionDeclined = true;
                m_agent->DeleteOffer(pchar->itemID());
                m_agent->SendMissionUpdate(call.client, "offer_declined");
                agentSays->SetItem(0, new PyInt(m_agent->GetDeclineRsp(pchar->itemID())));
                agentSays->SetItem(1, new PyInt(pchar->itemID()));
                /** @todo  add lp, etc, etc  */
                m_agent->UpdateStandings(call.client, Standings::MissionDeclined);
            } break;
            case Quit: {    //11
                missionQuit = true;
                MissionOffer offer = MissionOffer();
                m_agent->GetOffer(pchar->itemID(), offer);
                if (offer.courierTypeID) {
                    // remove item from player possession
                    call.client->RemoveMissionItem(offer.courierTypeID, offer.courierAmount);
                }
                // remove mission offer and set standings accordingly
                m_agent->DeleteOffer(pchar->itemID());
                m_agent->SendMissionUpdate(call.client, "quit");
                agentSays->SetItem(0, new PyInt(m_agent->GetDeclineRsp(pchar->itemID())));
                agentSays->SetItem(1, new PyInt(pchar->itemID()));
                /** @todo  add lp, etc, etc  */
                m_agent->UpdateStandings(call.client, Standings::MissionFailure, offer.important);
            } break;
            case Continue: {    //8
                // not sure what to do here.
            } break;
            case StartResearch: {   //12
                // not sure what to do here.
            } break;
            case CancelResearch: {  //13
                // not sure what to do here.
            } break;
            case BuyDatacores: {    //14
                // not sure what to do here.
            } break;
            case LocateCharacter:   //15
            case LocateAccept: {    //16
                // not sure what to do here.
            } break;
            case LocateReject: {    //17
                // not sure what to do here.
            } break;
            case Yes: {             //18
                // not sure what to do here.
            } break;
            case No: {              //19
                // not sure what to do here.
            } break;
            case AcceptChoice:{     //4
                // i think this is for options
            } break;
            case Admin: {           //20
                // not sure what to do here.
            } break;
            default: {
                // error
                _log(AGENT__ERROR, "AgentBound::Handle_DoAction() - unhandled buttonID %u", actionID );
                call.client->SendErrorMsg("Internal Server Error. Ref: ServerError xxxxx.");
                PySafeDecRef(dialog);
                PySafeDecRef(agentSays);
                return nullptr;
            }
        }
    } else {
        agentSays->SetItem(0, new PyInt(m_agent->GetStandingsRsp(pchar->itemID())));
        agentSays->SetItem(1, PyStatic.NewNone() /*new PyInt(pchar->itemID())*/);
    }

    // extraInfo data....
    PyDict* xtraInfo = new PyDict();
        xtraInfo->SetItemString("loyaltyPoints",    new PyInt(LPService::GetLPBalanceForCorp(pchar->itemID(),m_agent->GetCorpID())));  // this is char current LP
        xtraInfo->SetItemString("missionCompleted", new PyBool(missionCompleted));
        xtraInfo->SetItemString("missionQuit",      new PyBool(missionQuit));
        xtraInfo->SetItemString("missionDeclined",  new PyBool(missionDeclined));

    if (agentSays->empty()) {
        agentSays->SetItem(0, PyStatic.NewNone());  // briefingID
        agentSays->SetItem(1, PyStatic.NewNone());  // ContentID
    }
    PyTuple* inner = new PyTuple(2);
        inner->SetItem(0, agentSays);
        inner->SetItem(1, dialog);
    PyTuple* outer = new PyTuple(2);
        outer->SetItem(0, inner);
        outer->SetItem(1, xtraInfo);

    if (is_log_enabled(AGENT__RSP_DUMP)) {
        _log(AGENT__RSP_DUMP, "AgentBound::Handle_DoAction RSP:" );
        outer->Dump(AGENT__RSP_DUMP, "    ");
    }

    PySafeDecRef(agentSays);

    return outer;
}

namespace {
    /** Match Agent::m_offers, then global mission offers (agtOffers), same as GetMissionKeywords. */
    static bool ResolveOfferForClient(Agent* ag, uint32 charID, MissionOffer& offer) {
        if (ag->HasMission(charID, offer))
            return true;
        std::vector<MissionOffer> vec;
        sMissionDataMgr.LoadMissionOffers(charID, vec);
        for (const auto& o : vec) {
            if (o.agentID == ag->GetID()) {
                offer = o;
                return true;
            }
        }
        return false;
    }
}  // namespace

PyResult AgentBound::GetMissionBriefingInfo(PyCallArgs &call) {
    // called from iniate agent convo... should be populated when mission available
    // will return PyNone if no mission avalible
    _log(AGENT__MESSAGE,  "AgentBound::Handle_GetMissionBriefingInfo()");

    MissionOffer offer = MissionOffer();
    if (!ResolveOfferForClient(m_agent, call.client->GetCharacterID(), offer))
        return PyStatic.NewNone();

    // Offered/Accepted need briefing+keywords for the mission window. Returning None for Accepted
    // left the briefing/objectives panes empty after a courier is accepted.
    switch (offer.stateID) {
        case Mission::State::Failed:
        case Mission::State::Completed:
        case Mission::State::Rejected:
        case Mission::State::Defered:
        case Mission::State::Expired: {
            return PyStatic.NewNone();
        }
        default: break;
    }

    {
        const uint32 _ds0 = offer.destinationSystemID, _os0 = offer.originSystemID;
        RefillOfferSystemIds(offer, m_agent);
        if (_ds0 != offer.destinationSystemID or _os0 != offer.originSystemID)
            m_agent->UpdateOffer(call.client->GetCharacterID(), offer);
    }

    // these are found in the client data by MessageIDs ....  i.e.  {[location]objectiveDestinationID.name}
    // contentID is the key for the keywords data on live...not used here
    PyDict* keywords = new PyDict();
        keywords->SetItemString("objectiveLocationID", new PyInt(offer.originID));
        keywords->SetItemString("objectiveLocationSystemID", new PyInt(offer.originSystemID));
        keywords->SetItemString("objectiveTypeID", new PyInt(offer.courierTypeID));
        keywords->SetItemString("objectiveQuantity", new PyInt(offer.courierAmount));
        keywords->SetItemString("objectiveDestinationID", new PyInt(offer.destinationID));
        keywords->SetItemString("objectiveDestinationSystemID", new PyInt(offer.destinationSystemID));
        if (offer.rewardISK) {
            keywords->SetItemString("rewardTypeID", new PyInt(itemTypeCredits));
            keywords->SetItemString("rewardQuantity", new PyInt(offer.rewardISK));
        } else {
            // wouldnt these be in 'extra' or ?
            keywords->SetItemString("rewardTypeID", new PyInt(offer.rewardItemID));
            keywords->SetItemString("rewardQuantity", new PyInt(offer.rewardItemQty));
        }
        keywords->SetItemString("dungeonLocationID", new PyInt(offer.dungeonLocationID));
        keywords->SetItemString("dungeonSolarSystemID", new PyInt(offer.dungeonSolarSystemID));
        keywords->SetItemString("missionName", new PyString(offer.name.c_str()));
    PyDict *briefingInfo = new PyDict();
        briefingInfo->SetItemString("ContentID", new PyInt(offer.characterID));
        briefingInfo->SetItemString("Mission Keywords", keywords);
        if (offer.agentID == 90000007) {
            // Juro: do not send missionID as Mission Title ID (client treats it as a locale message id).
            // missionName + generic briefingID drive the window title; PyNone skips broken template chains (e.g. "4 of 10").
            briefingInfo->SetItemString("Mission Title ID", PyStatic.NewNone());
            briefingInfo->SetItemString("missionName", new PyString(offer.name.c_str()));
        } else {
            briefingInfo->SetItemString("Mission Title ID", new PyInt(offer.missionID));
        }
        briefingInfo->SetItemString("Mission Briefing ID", new PyInt(offer.briefingID));
        switch(offer.typeID) {
            case Mission::Type::Courier:
            case Mission::Type::Trade:
                briefingInfo->SetItemString("Mission Image", sMissionDataMgr.GetCourierRes()); break;
            case Mission::Type::Mining:
                briefingInfo->SetItemString("Mission Image", sMissionDataMgr.GetMiningRes()); break;
            case Mission::Type::Encounter:
                briefingInfo->SetItemString("Mission Image", sMissionDataMgr.GetKillRes()); break;
        }
        // decline time OR expiration time.  if not decline then expiration
        briefingInfo->SetItemString("Decline Time", PyStatic.NewNone());   // -1 is generic decline msg
        briefingInfo->SetItemString("Expiration Time", new PyLong(offer.expiryTime) );

    if (is_log_enabled(AGENT__RSP_DUMP)) {
        _log(AGENT__RSP_DUMP, "AgentBound::Handle_GetMissionBriefingInfo() RSP:" );
        briefingInfo->Dump(AGENT__RSP_DUMP, "    ");
    }

    return briefingInfo;
}

PyResult AgentBound::GetMissionKeywords(PyCallArgs &call, std::optional<PyInt*> a, std::optional<PyInt*> b,
    std::optional<PyInt*> c, std::optional<PyInt*> d, std::optional<PyInt*> e, std::optional<PyInt*> f) {
    // thse are the variables embedded in the messageIDs
    //self.missionArgs[contentID] = self.GetAgentMoniker(agentID).GetMissionKeywords(contentID)
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
    (void)f;
    _log(AGENT__MESSAGE, "AgentBound::GetMissionKeywords: tuple size %lli (mission keyword merge for msg 235549 etc.)", call.tuple->size());
    if (is_log_enabled(AGENT__DUMP)) {
        call.Dump(AGENT__DUMP);
    }
    MissionOffer offer = MissionOffer();
    bool haveOffer = ResolveOfferForClient(m_agent, call.client->GetCharacterID(), offer);

    if (haveOffer) {
        const uint32 _ds0 = offer.destinationSystemID, _os0 = offer.originSystemID;
        RefillOfferSystemIds(offer, m_agent);
        if (_ds0 != offer.destinationSystemID or _os0 != offer.originSystemID)
            m_agent->UpdateOffer(call.client->GetCharacterID(), offer);
    } else {
        offer.name.clear();
    }

    PyDict* keywords = new PyDict();
    keywords->SetItemString("objectiveLocationID", new PyInt(offer.originID));
    keywords->SetItemString("objectiveLocationSystemID", new PyInt(offer.originSystemID));
    keywords->SetItemString("objectiveTypeID", new PyInt(offer.courierTypeID));
    keywords->SetItemString("objectiveQuantity", new PyInt(offer.courierAmount));
    keywords->SetItemString("objectiveDestinationID", new PyInt(offer.destinationID));
    keywords->SetItemString("objectiveDestinationSystemID", new PyInt(offer.destinationSystemID));
    if (offer.rewardISK) {
        keywords->SetItemString("rewardTypeID", new PyInt(itemTypeCredits));
        keywords->SetItemString("rewardQuantity", new PyInt(offer.rewardISK));
    } else {
        // wouldnt these be in 'extra' or ?
        keywords->SetItemString("rewardTypeID", new PyInt(offer.rewardItemID));
        keywords->SetItemString("rewardQuantity", new PyInt(offer.rewardItemQty));
    }
    keywords->SetItemString("dungeonLocationID", new PyInt(offer.dungeonLocationID));
    keywords->SetItemString("dungeonSolarSystemID", new PyInt(offer.dungeonSolarSystemID));
    // Message 235549 is "{missionName} Objectives"; the client merges GetMissionKeywords into locale
    // kwargs. Without missionName, paths like ShowInfo -> BuildObjectiveHTML only pass session (player)
    // and GetByMessageID raises "Token has no value."
    keywords->SetItemString("missionName", new PyString(offer.name.c_str()));

    if (is_log_enabled(AGENT__RSP_DUMP)) {
        _log(AGENT__RSP_DUMP, "AgentBound::Handle_GetMissionKeywords() RSP:" );
        keywords->Dump(AGENT__RSP_DUMP, "    ");
    }

    return keywords;
}

PyResult AgentBound::GetMissionObjectiveInfo(PyCallArgs &call, std::optional <PyInt*> characterID, std::optional <PyInt*> contentID)
{
    // sends charID, contentID (although there's another call without any parameters, hence the optionals used)
    // returns PyDict loaded with mission info  or PyNone
    //  returning mission info sets double-pane view, where PyNone sets single-pane view
    _log(AGENT__DUMP,  "AgentBound::Handle_GetMissionObjectiveInfo() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    MissionOffer offer = MissionOffer();
    if (!m_agent->HasMission(call.client->GetCharacterID(), offer))
        return PyStatic.NewNone();

    // Same as GetMissionBriefingInfo: in-progress (Accepted) needs objective pane data.
    switch (offer.stateID) {
        //case Mission::State::Failed:
        case Mission::State::Completed:
        case Mission::State::Rejected:
        case Mission::State::Defered:
        case Mission::State::Expired: {
            return PyStatic.NewNone();
        }
        default: break;
    }

    return GetMissionObjectiveInfo(call.client, offer);
}

PyResult AgentBound::GetMyJournalDetails(PyCallArgs &call) {
    //parallelCalls.append((sm.GetService('agents').GetAgentMoniker(agentID).GetMyJournalDetails, ()))
    //missionState, importantMission, missionType, missionName, agentID, expirationTime, bookmarks, remoteOfferable, remoteCompletable = each
    // this is to update ONLY info with this agent....
    _log(AGENT__DUMP,  "AgentBound::Handle_GetMyJournalDetails() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    PyTuple *tuple = new PyTuple(2);
    //missions:
    PyList* missions = new PyList();
    MissionOffer offer = MissionOffer();
    if (m_agent->HasMission(call.client->GetCharacterID(), offer)) {
        if (offer.stateID < Mission::State::Completed) {
            PyTuple* mData = new PyTuple(9);
                mData->SetItem(0, new PyInt(offer.stateID)); //missionState  .. these may be wrong also.
                mData->SetItem(1, new PyInt(offer.important?1:0)); //importantMission  -- integer boolean
                mData->SetItem(2, new PyString(sMissionDataMgr.GetTypeLabelForAgent(offer.agentID, offer.typeID))); //missionTypeLabel
                mData->SetItem(3, new PyString(offer.name)); //missionName
                mData->SetItem(4, new PyInt(offer.agentID)); //agentID
                mData->SetItem(5, new PyLong(offer.expiryTime)); //expirationTime
                mData->SetItem(6, offer.bookmarks->Clone()); //bookmarks -- if populated, this is PyList of PyDicts as defined below...
                mData->SetItem(7, new PyBool(offer.remoteOfferable)); //remoteOfferable
                mData->SetItem(8, new PyBool(offer.remoteCompletable)); //remoteCompletable
            missions->AddItem(mData);
        }
    }
    tuple->SetItem(0, missions);

    //research:
    PyList* research = new PyList();
    tuple->SetItem(1, research);

    if (is_log_enabled(AGENT__RSP_DUMP))
        tuple->Dump(AGENT__RSP_DUMP, "   ");
    return tuple;
}

PyResult AgentBound::GetMissionJournalInfo(PyCallArgs &call, std::optional <PyInt*> characterID, std::optional <PyInt*> contentID) {
    //called on rclick in journal to "read details"
    //ret = self.GetAgentMoniker(agentID).GetMissionJournalInfo(charID, contentID)
    _log(AGENT__DUMP,  "AgentBound::Handle_GetMissionJournalInfo() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    MissionOffer offer = MissionOffer();
    if (!m_agent->HasMission(call.client->GetCharacterID(), offer))
        return PyStatic.NewNone();

    PyDict* journalInfo = new PyDict();
    journalInfo->SetItemString("contentID", new PyInt(offer.characterID));
    if (offer.agentID == 90000007) {
        // missionName from DB; missionID is not a client message id — avoid wrong locale titles.
        journalInfo->SetItemString("missionNameID", PyStatic.NewNone());
        journalInfo->SetItemString("missionName", new PyString(offer.name.c_str()));
    } else {
        journalInfo->SetItemString("missionNameID", new PyInt(offer.missionID));
    }
    journalInfo->SetItemString("briefingTextID", new PyInt(offer.briefingID));
    journalInfo->SetItemString("missionState", new PyInt(offer.stateID));
    journalInfo->SetItemString("expirationTime", new PyLong(offer.expiryTime) );
    journalInfo->SetItemString("objectives", GetMissionObjectiveInfo(call.client, offer));
    switch(offer.typeID) {
        case Mission::Type::Courier:
        case Mission::Type::Trade:
            journalInfo->SetItemString("missionImage", sMissionDataMgr.GetCourierRes()); break;
        case Mission::Type::Mining:
            journalInfo->SetItemString("missionImage", sMissionDataMgr.GetMiningRes()); break;
        case Mission::Type::Encounter:
            journalInfo->SetItemString("missionImage", sMissionDataMgr.GetKillRes()); break;
    }

    if (is_log_enabled(AGENT__RSP_DUMP)) {
        _log(AGENT__RSP_DUMP, "AgentBound::Handle_GetMissionJournalInfo() RSP:" );
        journalInfo->Dump(AGENT__RSP_DUMP, "    ");
    }

    return journalInfo;
}

PyDict* AgentBound::GetMissionObjectiveInfo(Client* pClient, MissionOffer& offer)
{
    {
        const uint32 _ds0 = offer.destinationSystemID, _os0 = offer.originSystemID;
        RefillOfferSystemIds(offer, m_agent);
        if (_ds0 != offer.destinationSystemID or _os0 != offer.originSystemID)
            m_agent->UpdateOffer(pClient->GetCharacterID(), offer);
    }
    PyDict* objectiveData = new PyDict();
    if (offer.agentID == 90000007) {
        // Do not use 235549 here: Crucible calls GetMissionObjectiveInfo then BuildObjectiveHTML with kwargs
        // from session only (no GetMissionKeywords), so GetByMessageID(235549) raises "Token has no value."
        objectiveData->SetItemString("missionTitleID", new PyInt(kClientMsg_OverviewObjectivesBlurb));
        objectiveData->SetItemString("missionName", new PyString(offer.name.c_str()));
    } else {
        objectiveData->SetItemString("missionTitleID", new PyInt(offer.missionID));
    }
    objectiveData->SetItemString("contentID", new PyInt(offer.characterID));
    objectiveData->SetItemString("importantStandings", new PyInt(offer.important));     // boolean integer
    // will need to test for this to set correctly.....
    if (pClient->IsMissionComplete(offer)) {     // Mission::Status:: data here 0=no, 1=yes, 2=cheat
        objectiveData->SetItemString("completionStatus", new PyInt(Mission::Status::Complete));
    } else {
        objectiveData->SetItemString("completionStatus", new PyInt(Mission::Status::Incomplete));
    }
    objectiveData->SetItemString("missionState", new PyInt(offer.stateID /*Mission::State::Offered*/));   // Mission::State:: data here for agentGift populating.  Accepted/failed to display gift items as accepted
    if (offer.agentID == 90000007 and offer.missionID == 58373)
        objectiveData->SetItemString("loyaltyPoints", new PyInt(0));
    else
        objectiveData->SetItemString("loyaltyPoints", new PyInt(offer.rewardLP));
    objectiveData->SetItemString("researchPoints", new PyInt(0));

    /*  this puts title/msg at bottom of right pane
    if (offer.stateID == Mission::State::Accepted)
        if (offer.typeID == Mission::Type::Courier) {
            PyTuple* missionExtra = new PyTuple(2);  // this is tuple(2)  headerID, bodyID    -- std locale msgIDs
                missionExtra->SetItem(0, new PyString("Reminder...."));   // this should be separate title from mission name
                missionExtra->SetItem(1, new PyString("Remember to get the %s from your hangar before you leave.", call.client->GetCourierItemRef(m_agent->GetID())->name()));   // this is additional info about mission, etc.
            objectiveData->SetItemString("missionExtra", missionExtra);
        } */

    PyList* locList = new PyList();    // tuple of list of locationIDs (pickup and dropoff)
        locList->AddItem(new PyInt(offer.originSystemID));
        locList->AddItem(new PyInt(offer.destinationSystemID));
    objectiveData->SetItemString("locations", locList);

    PyList* giftList = new PyList();    // this is list of tuple(3)  typeID, quantity, extra
    /*
    PyDict* extra = new PyDict();    // 'extra' is either specificItemID or blueprint data.
        extra->SetItemString("specificItemID", PyStatic.NewNone());
        extra->SetItemString("blueprintInfo", PyStatic.NewNone());
    PyTuple* agentGift = new PyTuple(3);
        agentGift->SetItem(0, PyStatic.NewNone());
        agentGift->SetItem(1, PyStatic.NewNone());
        agentGift->SetItem(2, extra);
        giftList->AddItem(agentGift);
    */
    objectiveData->SetItemString("agentGift", giftList);

    PyList* normList = new PyList();    // this is list of tuple(3)  typeID, quantity, extra
    if (offer.rewardISK) {
        PyDict* extra = new PyDict();    // 'extra' is either specificItemID or blueprint data.
        PyTuple* normalRewards = new PyTuple(3);
        normalRewards->SetItem(0, new PyInt(itemTypeCredits));
        normalRewards->SetItem(1, new PyInt(offer.rewardISK));
        normalRewards->SetItem(2, extra);
        normList->AddItem(normalRewards);
    }
    if (offer.rewardItemID) {
        PyDict* extra = new PyDict();
        PyTuple* normalRewards = new PyTuple(3);
        normalRewards->SetItem(0, new PyInt(offer.rewardItemID));
        normalRewards->SetItem(1, new PyInt(offer.rewardItemQty));
        normalRewards->SetItem(2, extra);
        normList->AddItem(normalRewards);
    }
    if (offer.agentID == 90000007 and offer.missionID == 58373 and offer.rewardExtraItemID) {
        PyDict* extraBp = new PyDict();
        PyTuple* bpReward = new PyTuple(3);
        bpReward->SetItem(0, new PyInt(offer.rewardExtraItemID));
        bpReward->SetItem(1, new PyInt(1));
        bpReward->SetItem(2, extraBp);
        normList->AddItem(bpReward);
    }
    objectiveData->SetItemString("normalRewards", normList);

    PyList* collateralList = new PyList(); // this is list of tuple(3)  typeID, quantity, extra
    /*
    PyDict* extra = new PyDict();    // 'extra' is either specificItemID or blueprint data.
        extra->SetItemString("specificItemID", PyStatic.NewNone());
        extra->SetItemString("blueprintInfo", PyStatic.NewNone());
    PyTuple* collateral = new PyTuple(3);
        collateral->SetItem(0, PyStatic.NewNone());
        collateral->SetItem(1, PyStatic.NewNone());
        collateral->SetItem(2, extra);
        */
    objectiveData->SetItemString("collateral", collateralList);

    PyList* bonusList = new PyList();   // this is list of tuple(4)  timeRemaining, typeID, quantity, extra
    if (offer.bonusTime > 0) {
        PyDict* extra = new PyDict();
        PyTuple* bonusRewards = new PyTuple(4);
        int64_t remaining = (int64_t)offer.bonusTime * (int64_t)EvE::Time::Minute;
        if (offer.dateAccepted > 0) {
            const int64_t deadline = offer.dateAccepted + (int64_t)offer.bonusTime * (int64_t)EvE::Time::Minute;
            remaining = deadline - GetFileTimeNow();
            if (remaining < 0)
                remaining = 0;
        }
        bonusRewards->SetItem(0, new PyLong(remaining));
        bonusRewards->SetItem(1, new PyInt(itemTypeCredits));
        bonusRewards->SetItem(2, new PyInt(offer.bonusISK));
        bonusRewards->SetItem(3, extra);
        bonusList->AddItem(bonusRewards);
    }
    // bonusList can be multiple items, usualy only item or isk for time bonus
    if (false/*bonus2*/) {
        PyDict* extra = new PyDict();    // 'extra' is either specificItemID or blueprint data.
            //extra->SetItemString("specificItemID", PyStatic.NewNone());
            //extra->SetItemString("blueprintInfo", PyStatic.NewNone());
        PyTuple* bonusRewards2 = new PyTuple(4);
            bonusRewards2->SetItem(0, new PyLong(12000000000)); //20m
            bonusRewards2->SetItem(1, new PyInt(itemTypeTrit));
            bonusRewards2->SetItem(2, new PyInt(offer.rewardISK));
            bonusRewards2->SetItem(3, extra);
        bonusList->AddItem(bonusRewards2);
    }
    objectiveData->SetItemString("bonusRewards", bonusList);
    /*  for collateral and rewards, as follows...
    typeID, quantity, extra in objectiveData['normalRewards']
    typeID, quantity, extra in objectiveData['collateral']
    typeID, quantity, extra in objectiveData['agentGift']
    or
    timeRemaining, typeID, quantity, extra in objectiveData['bonusRewards']

        specificItemID = extra.get('specificItemID', 0)
        blueprintInfo = extra.get('blueprintInfo', None)
        */

    objectiveData->SetItemString("objectives", GetMissionObjectives(pClient, offer));
    PyList* dunList = new PyList();
    if (offer.typeID == Mission::Type::Encounter and offer.dungeonLocationID != 0) {
        PyDict* dunData = new PyDict();
        dunData->SetItemString("dungeonID", new PyInt(offer.dungeonLocationID));
        dunData->SetItemString("completionStatus", new PyInt(Dungeon::Status::Started));
        dunData->SetItemString("optional", new PyInt(0));
        dunData->SetItemString("briefingMessage", new PyInt(0));
        dunData->SetItemString("objectiveCompleted", new PyBool(false));
        dunData->SetItemString("ownerID", new PyInt(m_agent->GetID()));
        dunData->SetItemString("shipRestrictions", new PyInt(0));
        dunData->SetItemString("location", m_agent->GetLocationWrap());
        dunList->AddItem(dunData);
    }
    objectiveData->SetItemString("dungeons", dunList);
    /* dunData data....
     * dungeonID
     * completionStatus
     * optional
     * briefingMessage
     * objectiveCompleted
     * ownerID
     * location
        location['locationID']
        location['locationType']
        location['solarsystemID']
        location['coords']
        location['agentID']
        ?location['referringAgentID']
        ?location['shipTypeID']
     * shipRestrictions  0=normal 1=special with link to *something else*
     */

    if (is_log_enabled(AGENT__RSP_DUMP)) {
        _log(AGENT__RSP_DUMP, "AgentBound::Handle_GetMissionObjectiveInfo() RSP:" );
        objectiveData->Dump(AGENT__RSP_DUMP, "    ");
    }

    return objectiveData;
}

PyTuple* AgentBound::GetMissionObjectives(Client* pClient, MissionOffer& offer)
{
    // set mission objectiveData based on mission type.
    {
        const uint32 _ds0 = offer.destinationSystemID, _os0 = offer.originSystemID;
        RefillOfferSystemIds(offer, m_agent);
        if (_ds0 != offer.destinationSystemID or _os0 != offer.originSystemID)
            m_agent->UpdateOffer(pClient->GetCharacterID(), offer);
    }
    PyDict* dropoffLocation = new PyDict();
    uint32 destLoc   = offer.destinationID;
    uint32 destType  = offer.destinationTypeID;
    uint32 destSys   = offer.destinationSystemID;

    // Courier/trade: client `transport` row requires `locationID` (station). Resolve solar system
    // ids 30M) to 60M station ids. Static m_stationList can miss systems; then use staStations.
    if (offer.typeID == Mission::Type::Courier or offer.typeID == Mission::Type::Trade
        or offer.typeID == Mission::Type::Mining) {
        const auto loadStations = [](uint32 solarSystemId, std::vector<uint32>& stList) -> bool {
            stList.clear();
            if (sDataMgr.GetStationList(solarSystemId, stList) and (not stList.empty()))
                return true;
            return StationDB::GetStationsInSolarSystem(solarSystemId, stList);
        };
        const auto pickFromStations = [&](std::vector<uint32>& stList) -> void {
            if (stList.empty())
                return;
            destLoc = stList[0];
            for (uint32 sid : stList) {
                if (sid != m_agent->GetStationID()) {
                    destLoc = sid;
                    break;
                }
            }
            StationData data = StationData();
            if (stDataMgr.GetStationData(destLoc, data)) {
                destType  = data.typeID;
                destSys   = data.systemID;
            } else {
                if (destSys == 0) {
                    destSys = sDataMgr.GetStationSystem(destLoc);
                    if (destSys == 0)
                        destSys = StationDB::GetSolarSystemIDForStation(destLoc);
                }
            }
        };
        if (not (IsStationID(destLoc) and destLoc != 0)) {
            // destinationID is a solar system
            if (IsSolarSystemID(destLoc) and destLoc != 0) {
                std::vector<uint32> stList;
                if (loadStations(destLoc, stList)) pickFromStations(stList);
            }
            // only system (or 0) in destinationID — try known systems from the offer
            if (not (IsStationID(destLoc) and destLoc != 0) and offer.destinationSystemID != 0) {
                std::vector<uint32> stList;
                if (loadStations(offer.destinationSystemID, stList)) pickFromStations(stList);
            }
            if (not (IsStationID(destLoc) and destLoc != 0) and offer.originSystemID != 0) {
                std::vector<uint32> stList;
                if (loadStations(offer.originSystemID, stList)) pickFromStations(stList);
            }
        }
        // Last resort: use agent's station for UI — never for long-haul couriers or the client shows wrong dropoff
        if (not (IsStationID(destLoc) and destLoc != 0) and
            offer.range != Agents::Range::DistantLowSecEightToFifteenJumps
            and offer.range != Agents::Range::WithinThirteenJumpsOfAgent) {
            const uint32 agentSt = m_agent->GetStationID();
            if (IsStationID(agentSt) and agentSt != 0) {
                destLoc  = agentSt;
                destSys  = 0;
                destType = m_agent->GetLocTypeID();
                StationData data = StationData();
                if (stDataMgr.GetStationData(destLoc, data)) {
                    destType  = data.typeID;
                    destSys   = data.systemID;
                } else {
                    destSys   = sDataMgr.GetStationSystem(destLoc);
                    if (destSys == 0)
                        destSys = StationDB::GetSolarSystemIDForStation(destLoc);
                }
            }
        }
    }

    if (IsStationID(destLoc) and destLoc != 0) {
        if (destSys == 0) {
            destSys = sDataMgr.GetStationSystem(destLoc);
            if (destSys == 0)
                destSys = StationDB::GetSolarSystemIDForStation(destLoc);
        }
        dropoffLocation->SetItemString("typeID", new PyInt(destType) );
        dropoffLocation->SetItemString("locationID", new PyInt(destLoc) );
        dropoffLocation->SetItemString("solarsystemID", new PyInt(destSys) );
    } else {
        dropoffLocation->SetItemString("shipTypeID", new PyInt(offer.destinationTypeID) );
        dropoffLocation->SetItemString("agentID", new PyInt(offer.destinationOwnerID) );
        // get agent in space location and set here
        PyTuple* coords = new PyTuple(3);
            coords->SetItem(0, new PyFloat(0)); //x
            coords->SetItem(1, new PyFloat(0)); //y
            coords->SetItem(2, new PyFloat(0)); //z
        dropoffLocation->SetItemString("coords", coords);
        dropoffLocation->SetItemString("referringAgentID", new PyInt(offer.agentID) );
    }

    PyTuple* objectives = new PyTuple(1);
    switch (offer.typeID) {
        case Mission::Type::Trade:
        case Mission::Type::Courier: {
            PyDict* pickupLocation = new PyDict();
            {
                uint32 oSys = offer.originSystemID;
                if (IsStationID(offer.originID) and oSys == 0) {
                    oSys = sDataMgr.GetStationSystem(offer.originID);
                    if (oSys == 0)
                        oSys = StationDB::GetSolarSystemIDForStation(offer.originID);
                }
                pickupLocation->SetItemString("typeID", new PyInt(m_agent->GetLocTypeID()) );
                pickupLocation->SetItemString("locationID", new PyInt(offer.originID) );
                pickupLocation->SetItemString("solarsystemID", new PyInt(oSys) );
            }
            PyDict* cargo = new PyDict();
                cargo->SetItemString("hasCargo", new PyBool(pClient->ContainsTypeQty(offer.courierTypeID, offer.courierAmount)));
                cargo->SetItemString("typeID", new PyInt(offer.courierTypeID));
                cargo->SetItemString("quantity", new PyInt(offer.courierAmount));
                cargo->SetItemString("volume", new PyFloat(offer.courierItemVolume * offer.courierAmount));    // calculated shipment volume.  *this is direct to window*
            PyTuple* objData = new PyTuple(5);
                objData->SetItem(0, new PyInt(offer.originOwnerID));
                objData->SetItem(1, pickupLocation/*m_agent->GetLocationWrap()*/);
                objData->SetItem(2, new PyInt(offer.destinationOwnerID));
                objData->SetItem(3, dropoffLocation/*m_agent->GetLocationWrap()*/);
                objData->SetItem(4, cargo);
            PyTuple* objType = new PyTuple(2);   // this is list of tuple(2)    objType, objData
                objType->SetItem(0, new PyString("transport"));
                objType->SetItem(1, objData);
            objectives->SetItem(0, objType);
        } break;
        case Mission::Type::Encounter:
        case Mission::Type::Mining: {
            PyDict* cargo = new PyDict();
                cargo->SetItemString("hasCargo", new PyBool(pClient->ContainsTypeQty(offer.courierTypeID, offer.courierAmount)));
                cargo->SetItemString("typeID", new PyInt(offer.courierTypeID));
                cargo->SetItemString("quantity", new PyInt(offer.courierAmount));
                cargo->SetItemString("volume", new PyFloat(offer.courierItemVolume * offer.courierAmount));    // calculated shipment volume.  *this is direct to window*
            PyTuple* objData = new PyTuple(3);
                objData->SetItem(0, new PyInt(offer.destinationOwnerID));
                objData->SetItem(1, dropoffLocation/*m_agent->GetLocationWrap()*/);
                objData->SetItem(2, cargo);
            PyTuple* objType = new PyTuple(2);   // this is list of tuple(2)    objType, objData
                objType->SetItem(0, new PyString("fetch"));
                objType->SetItem(1, objData);
            objectives->SetItem(0, objType);
        } break;
        case Mission::Type::Anomic:
        case Mission::Type::EpicArc:
        case Mission::Type::Burner:
        case Mission::Type::Cosmos:
        case Mission::Type::Data:
        case Mission::Type::Research:
        case Mission::Type::Storyline:
        case Mission::Type::Tutorial: {
            objectives->SetItem(0, PyStatic.NewNone());
        } break;
    }

    // cleanup
    PySafeDecRef(dropoffLocation);

    return objectives;

    /*  objectives data...
    if objType == 'agent':      -- report to agent
        agentID, agentLocation = objData
        agentLocation['locationID']
        agentLocation['locationType']
        agentLocation['solarsystemID']
        if not in station
            agentLocation['coords']
            agentLocation['agentID']
            ?agentLocation['referringAgentID']
            ?agentLocation['shipTypeID']

    elif objType == 'transport':        -- courier and trade missions
        pickupOwnerID, pickupLocation, dropoffOwnerID, dropoffLocation, cargo = objData
        pickupLocation['locationID']
        pickupLocation['locationType']
        pickupLocation['solarsystemID']
        dropoffLocation['locationID']
        dropoffLocation['locationType']
        dropoffLocation['solarsystemID']
        if not in station
            dropoffLocation['coords']
            dropoffLocation['agentID']
            ?dropoffLocation['referringAgentID']
            ?dropoffLocation['shipTypeID']
        cargo['hasCargo']
        cargo['typeID']
        cargo['volume']
        cargo['quantity']

    elif objType == 'fetch':            -- encounter and mining missions
        dropoffOwnerID, dropoffLocation, cargo = objData
        dropoffLocation['locationID']
        dropoffLocation['locationType']
        dropoffLocation['solarsystemID']
        if not in station
            dropoffLocation['coords']
            dropoffLocation['agentID']
            ?dropoffLocation['referringAgentID']
            ?dropoffLocation['shipTypeID']
        cargo['hasCargo']
        cargo['typeID']
        cargo['volume']
        cargo['quantity']
        */
}


/**     ***********************************************************************
 * @note   these do absolutely nothing at this time....
 */

PyResult AgentBound::GetDungeonShipRestrictions(PyCallArgs &call, PyInt* dungeonID) {
    (void)dungeonID;
    _log(AGENT__DUMP,  "AgentBound::Handle_GetDungeonShipRestrictions() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    return new PyDict();
}

PyResult AgentBound::RemoveOfferFromJournal(PyCallArgs &call) {
    //called on rclick in journal to "remove offer"
    //self.GetAgentMoniker(agentID).RemoveOfferFromJournal()
    _log(AGENT__DUMP,  "AgentBound::Handle_RemoveOfferFromJournal() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    return nullptr;
}

PyResult AgentBound::GetOfferJournalInfo(PyCallArgs &call) {
    //html = self.GetAgentMoniker(agentID).GetOfferJournalInfo()
    _log(AGENT__DUMP,  "AgentBound::Handle_GetOfferJournalInfo() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    return nullptr;
}

PyResult AgentBound::GetEntryPoint(PyCallArgs &call) {
    //entryPoint = sm.StartService('agents').GetAgentMoniker(bookmark.agentID).GetEntryPoint()
    _log(AGENT__DUMP,  "AgentBound::Handle_GetEntryPoint() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    return nullptr;
}

PyResult AgentBound::GotoLocation(PyCallArgs &call, PyInt* locationType, PyInt* locationNumber, PyInt* referringAgentID) {
    //sm.StartService('agents').GetAgentMoniker(bookmark.agentID).GotoLocation(bookmark.locationType, bookmark.locationNumber, referringAgentID)
    _log(AGENT__DUMP,  "AgentBound::Handle_GotoLocation() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    return nullptr;
}

PyResult AgentBound::WarpToLocation(PyCallArgs &call, PyInt* locationType, PyInt* locationNumber, PyFloat* warpRange, PyBool* fleet, PyInt* referringAgentID) {
    //sm.StartService('agents').GetAgentMoniker(bookmark.agentID).WarpToLocation(bookmark.locationType, bookmark.locationNumber, warpRange, fleet, referringAgentID)
    _log(AGENT__DUMP,  "AgentBound::Handle_WarpToLocation() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    return nullptr;
}
