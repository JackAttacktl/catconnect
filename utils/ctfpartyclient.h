/* Copied from cathook and adapted for windows
 *
 * CTFPartyClient.cpp
 *
 *  Created on: Dec 7, 2017
 *      Author: nullifiedcat
 */

#ifndef _CTFPARTYCLIENT_RECLASS_INC_
#define _CTFPARTYCLIENT_RECLASS_INC_

#include <stdint.h>
#include <vector>
#include "isteamclient.h"
#include "defs.h"
#include "sigscan.h"
#include "xorstr.h"
#include "e8call.h"

namespace NSReclass
{
    class ITFGroupMatchCriteria;
    class ITFMatchGroupDescription;

    class CTFPartyClient
    {
     public:
        FORCEINLINE static CTFPartyClient * GTFPartyClient()
        {
            typedef CTFPartyClient *(__cdecl * GTFPartyClient_t)();
            //static CTFPartyClient * pAddr = *(CTFPartyClient **)(NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\xA1\x2A\x2A\x2A\x2A\xC3\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x55\x8B\xEC\x83\xEC\x4C\x53"), 23) + 1);
            static GTFPartyClient_t pFunc = (GTFPartyClient_t)e8call((NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x81\xEC\x10\x02\x00\x00\x56\x8B\xF1\x89\x75\xFC\xE8"), 16) + 16));
            return pFunc();
        }

        //use BInStandbyQueue and BInQueueForMatchGroup cuz they are more safe
        //bool BInQueue() { return *(uint8_t *)((uint8_t *)this + 73); }

        //aka GetNumOnlineMembers in cathook
        FORCEINLINE int CountNumOnlinePartyMembers()
        {
            typedef int (__thiscall * CountNumOnlinePartyMembersFn_t)(CTFPartyClient *);
            static CountNumOnlinePartyMembersFn_t CountNumOnlinePartyMembersFn = nullptr;
            if (!CountNumOnlinePartyMembersFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x1C\x8B\xC1\x53\x89\x45\xFC"), 12);
                CountNumOnlinePartyMembersFn = CountNumOnlinePartyMembersFn_t(pAddr);
            }
            return CountNumOnlinePartyMembersFn(this);
        }

        //aka GetNumMembers in cathook
        FORCEINLINE int GetNumPartyMembers()
        {
            typedef int (__thiscall * GetNumPartyMembers_t)(CTFPartyClient *);
            static GetNumPartyMembers_t GetNumPartyMembersFn = nullptr;
            if (!GetNumPartyMembersFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x8B\x49\x30\x85\xC9\x74\x2A\x81\xC1\x90\x00\x00\x00"), 13);
                GetNumPartyMembersFn = GetNumPartyMembers_t(pAddr);
            }
            return GetNumPartyMembersFn(this);
        }

        FORCEINLINE void SendPartyChat(const char * pMessage)
        {
            typedef void (__thiscall * SendPartyChat_t)(CTFPartyClient *, const char *);
            static SendPartyChat_t SendPartyChatFn = nullptr;
            if (!SendPartyChatFn)
            {
                void * pAddr = (void *)e8call(NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x8B\x45\x08\x83\x38\x02\x74\x2A\xC7\x45\x08\x2A\x2A\x2A\x2A\x5D\xFF\x25\x2A\x2A\x2A\x2A\xFF\xB0\x0C\x04\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\xC8\xE8"), 39) + 39);
                SendPartyChatFn = SendPartyChat_t(pAddr);
            }
            SendPartyChatFn(this, pMessage);
        }

        FORCEINLINE bool BCanQueueForStandby()
        {
            typedef bool (__thiscall * BCanQueueForStandby_t)(CTFPartyClient *);
            static BCanQueueForStandby_t BCanQueueForStandbyFn = nullptr;
            if (!BCanQueueForStandbyFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x14\x56\x6A\x00\x8B\xF1\xE8\x2A\x2A\x2A\x2A\x84\xC0"), 18);
                BCanQueueForStandbyFn = BCanQueueForStandby_t(pAddr);
            }
            return BCanQueueForStandbyFn(this);
        }

        FORCEINLINE ITFGroupMatchCriteria * MutLocalGroupCriteria()
        {
            typedef ITFGroupMatchCriteria * (__thiscall * MutLocalGroupCriteria_t)(CTFPartyClient *);
            static MutLocalGroupCriteria_t MutLocalGroupCriteriaFn = nullptr;
            if (!MutLocalGroupCriteriaFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x83\x79\x30\x00\xC6\x81\x24\x02\x00\x00\x01\x74\x2A\x80\x79\x40\x00\x74\x2A\xC6\x81\x48\x02\x00\x00\x01\x8D\x81\x3C\x01\x00\x00"), 32);
                MutLocalGroupCriteriaFn = MutLocalGroupCriteria_t(pAddr);
            }
            return MutLocalGroupCriteriaFn(this);
        }

        FORCEINLINE int LoadSavedCasualCriteria()
        {
            typedef int (__thiscall * LoadSavedCasualCriteria_t)(CTFPartyClient *);
            static LoadSavedCasualCriteria_t LoadSavedCasualCriteriaFn = nullptr;
            if (!LoadSavedCasualCriteriaFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x83\x79\x30\x00\xC6\x81\x24\x02\x00\x00\x01\x74\x2A\x80\x79\x40\x00"
                                                                                        "\x74\x2A\xC6\x81\x48\x02\x00\x00\x01\xFF\x35\x2A\x2A\x2A\x2A\x81"), 33);
                LoadSavedCasualCriteriaFn = LoadSavedCasualCriteria_t(pAddr);
            }
            return LoadSavedCasualCriteriaFn(this);
        }

        FORCEINLINE void RequestQueueForStandby()
        {
            typedef void (__thiscall * RequestQueueForStandby_t)(CTFPartyClient *);
            static RequestQueueForStandby_t RequestQueueForStandbyFn = nullptr;
            if (!RequestQueueForStandbyFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x18\x56\x8B\xF1\x83\x7E\x30\x00"), 13);
                RequestQueueForStandbyFn = RequestQueueForStandby_t(pAddr);
            }
            RequestQueueForStandbyFn(this);
        }

        FORCEINLINE char RequestQueueForMatch(int iType)
        {
            typedef char (__thiscall * RequestQueueForMatch_t)(CTFPartyClient *, int);
            static RequestQueueForMatch_t RequestQueueForMatchFn = nullptr;
            if (!RequestQueueForMatchFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x38\x56\x8B\x75\x08\x57"), 11);
                RequestQueueForMatchFn = RequestQueueForMatch_t(pAddr);
            }
            return RequestQueueForMatchFn(this, iType);
        }

        FORCEINLINE bool BInQueueForMatchGroup(int iType)
        {
            typedef bool (__thiscall * BInQueueForMatchGroup_t)(CTFPartyClient*, int);
            static BInQueueForMatchGroup_t BInQueueForMatchGroupFn = nullptr;
            if (!BInQueueForMatchGroupFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x56\x8B\x75\x08\x57\x56\x8B\xF9\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x84"
                                                                                        "\xC0\x74\x2A\x83\xFE\xFF\x74\x2A\x8B\x57\x50\x33\xC0\x85\xD2\x7E\x2A\x8B\x4F\x44"
                                                                                        "\x39\x31\x74\x2A\x40\x83\xC1\x04\x3B\xC2\x7C\x2A\x83\xC8\xFF\x83\xF8\xFF\x5F"), 59);
                BInQueueForMatchGroupFn = BInQueueForMatchGroup_t(pAddr);
            }
            return BInQueueForMatchGroupFn(this, iType);
        }

        FORCEINLINE bool BInStandbyQueue() { return *((uint8_t *)this + 88); }

        //aka RequestLeaveForMatch in cathook
        FORCEINLINE char CancelMatchQueueRequest(int iType)
        {
            typedef char (__thiscall * CancelMatchQueueRequest_t)(CTFPartyClient *, int);
            static CancelMatchQueueRequest_t CancelMatchQueueRequestFn = nullptr;
            if (!CancelMatchQueueRequestFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x18\x53\x56\x8B\x75\x08\x8B\xD9\x56"), 14);
                CancelMatchQueueRequestFn = CancelMatchQueueRequest_t(pAddr);
            }
            return CancelMatchQueueRequestFn(this, iType);
        }

        FORCEINLINE int BInvitePlayerToParty(CSteamID sSteamID, bool bUnknown = false) //bool somehow related to steamapi
        {
            typedef int (__thiscall * BInvitePlayerToParty_t)(CTFPartyClient *, CSteamID, bool);
            static BInvitePlayerToParty_t BInvitePlayerToPartyFn = nullptr;
            if (!BInvitePlayerToPartyFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x81\xEC\x28\x01\x00\x00\x56\xFF\x75\x0C"), 13);
                BInvitePlayerToPartyFn = BInvitePlayerToParty_t(pAddr);
            }
            return BInvitePlayerToPartyFn(this, sSteamID, bUnknown);
        }

        FORCEINLINE int BRequestJoinPlayer(CSteamID sSteamID, bool bUnknown = false)
        {
            typedef int (__thiscall * BRequestJoinPlayer_t)(CTFPartyClient *, CSteamID, bool);
            static BRequestJoinPlayer_t BRequestJoinPlayerFn = nullptr;
            if (!BRequestJoinPlayerFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x14\x56\xFF\x75\x0C"), 10);
                BRequestJoinPlayerFn = BRequestJoinPlayer_t(pAddr);
            }
            return BRequestJoinPlayerFn(this, sSteamID, bUnknown);
        }

        //aka PromotePlayerToLeader in cathook
        FORCEINLINE int BPromoteToLeader(CSteamID sSteamID)
        {
            typedef int (__thiscall * BPromoteToLeader_t)(CTFPartyClient *, CSteamID);
            static BPromoteToLeader_t BPromoteToLeaderFn = nullptr;
            if (!BPromoteToLeaderFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x14\x56\x8B\xF1\x8B\x4E\x30\x85\xC9\x0F"
                                                                                        "\x84\x2A\x2A\x2A\x2A\x80\x7E\x40\x00\x0F\x84\x2A\x2A\x2A\x2A"
                                                                                        "\x85\xC9\x0F\x84\x2A\x2A\x2A\x2A\x8B\x81\x90\x00\x00\x00\x8D"
                                                                                        "\x55\x08\x81\xC1\x90\x00\x00\x00\x52\xFF\x50\x10\x83\xF8\xFF"
                                                                                        "\x0F\x84\x2A\x2A\x2A\x2A\x68\xAF\x19\x00\x00"), 71);
                BPromoteToLeaderFn = BPromoteToLeader_t(pAddr);
            }
            return BPromoteToLeaderFn(this, sSteamID);
        }

        FORCEINLINE std::vector<unsigned int> GetPartySteamIDs()
        {
            typedef bool (__cdecl * SteamIDOfPartySlot_t)(int iSlot, CSteamID * pOut);
            static SteamIDOfPartySlot_t SteamIDOfSlotFn = nullptr;
            if (!SteamIDOfSlotFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x08\xE8\x2A\x2A\x2A\x2A\x83\x78\x30\x00"), 15);
                SteamIDOfSlotFn = SteamIDOfPartySlot_t(pAddr);
            }
            std::vector<unsigned int> vPartyMembers;
            for (int i = 0; i < GetNumPartyMembers(); i++)
            {
                CSteamID sOut;
                SteamIDOfSlotFn(i, &sOut);
                if (sOut.GetAccountID()) vPartyMembers.push_back(sOut.GetAccountID());
            }
            return vPartyMembers;
        }

        //aka KickPlayer in cathook
        FORCEINLINE int BKickPartyMember(CSteamID sSteamID)
        {
            typedef int (__thiscall * KickPlayer_t)(CTFPartyClient *, CSteamID);
            static KickPlayer_t KickPlayerFn = nullptr;
            if (!KickPlayerFn)
            {
                void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x14\x56\x8B\xF1\x8B\x4E\x30\x85"
                                                                                        "\xC9\x0F\x84\x2A\x2A\x2A\x2A\x80\x7E\x40\x00\x0F\x84"
                                                                                        "\x2A\x2A\x2A\x2A\x85\xC9\x0F\x84\x2A\x2A\x2A\x2A\x8B"
                                                                                        "\x81\x90\x00\x00\x00\x8D\x55\x08\x81\xC1\x90\x00\x00"
                                                                                        "\x00\x52\xFF\x50\x10\x83\xF8\xFF\x0F\x84\x2A\x2A\x2A"
                                                                                        "\x2A\x68\xB0\x19\x00\x00"), 71);
                KickPlayerFn = KickPlayer_t(pAddr);
            }
            return KickPlayerFn(this, sSteamID);
        }

        //I'm not sure about this function
        FORCEINLINE bool GetCurrentPartyLeader(CSteamID& sID)
        {
            void * pParty = *reinterpret_cast<void **>((char *)this + 0x34);
            if (!pParty) return false;
            sID = *reinterpret_cast<CSteamID *>((char *)pParty + 0x20);
            return true;
        }
    };

    FORCEINLINE ITFMatchGroupDescription * GetMatchGroupDescription(int& iIDX)
    {
        typedef ITFMatchGroupDescription * (__cdecl * GetMatchGroupDescription_t)(int&);
        static GetMatchGroupDescription_t GetMatchGroupDescriptionFn = nullptr;
        if (!GetMatchGroupDescriptionFn)
        {
            void * pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x8B\x45\x08\x8B\x00\x83\xF8\xFF"), 11);
            GetMatchGroupDescriptionFn = GetMatchGroupDescription_t(pAddr);
        }
        return GetMatchGroupDescriptionFn(iIDX);
    }
}

#endif