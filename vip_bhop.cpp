/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * VIP Bhop
 * Written by Phoenix (˙·٠●Феникс●٠·˙) 2024.
 * ======================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 */

#include "vip_bhop.h"
#include <entitysystem.h>
#include <convar.h>
#include <networksystem/inetworkserializer.h>
#include <networksystem/inetworkmessages.h>
#include <inetchannel.h>
#include <networkbasetypes.pb.h>
#include "schemasystem_helper.h"
#include "sdk/CCSPlayer_MovementServices.h"
#include "utils.hpp"
#include "module.h"
#include <memory>

const constexpr char g_szFeature[] = "bhop";

VIPBhop g_VIPBhop;
PLUGIN_EXPOSE(VIPBhop, g_VIPBhop);
IVIPApi* g_pVIPCore;

IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
ConVar* sv_autobunnyhopping;
bool g_bBhop[64] = {};

void SetPlayerConVar(CPlayerSlot nSlot, const char* name, const char* value);
void (*vCCSPlayer_MovementServices_CheckJumpPre)(CCSPlayer_MovementServices* _this, void* mv);
void CCSPlayer_MovementServices_CheckJumpPre(CCSPlayer_MovementServices* _this, void* mv);
void OnConVarChanged(ConVarRefAbstract* cvar, CSplitScreenSlot nSlot, const char* pNewValue, const char* pOldValue);


bool VIPBhop::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION)
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);

	DynLibUtils::CModule libserver(g_pSource2Server);

	std::unique_ptr<subhook::Hook> upCCSPlayer_MovementServices_CheckJumpPre;
	
	// void CCSPlayer_MovementServices::CheckJumpPre(CCSPlayer_MovementServices* _this, CMoveData * mv)
	{
		DynLibUtils::CMemory pCheckJumpPre = libserver.FindPattern(WIN_LINUX("48 89 5C 24 ? 56 48 83 EC 40 48 8B F2 48 8B D9 BA ? ? ? ? E8 ? ? ? ? 48 8B 4B 30", "55 48 89 E5 41 56 41 55 41 54 49 89 F4 BE ? ? ? ? 53 48 89 FB 48 83 EC 30 E8 ? ? ? ? 48 8B 7B 30"));
		if (!pCheckJumpPre)
		{
			V_strncpy(error, "Failed to find CCSPlayer_MovementServices::CheckJumpPre", maxlen);
			return false;
		}
	
		upCCSPlayer_MovementServices_CheckJumpPre = std::make_unique<subhook::Hook>(pCheckJumpPre, reinterpret_cast<void*>(CCSPlayer_MovementServices_CheckJumpPre), subhook::HookFlagTrampoline | subhook::HookFlag64BitOffset);
		vCCSPlayer_MovementServices_CheckJumpPre = reinterpret_cast<decltype(vCCSPlayer_MovementServices_CheckJumpPre)>(upCCSPlayer_MovementServices_CheckJumpPre->GetTrampoline());
		if (!vCCSPlayer_MovementServices_CheckJumpPre)
		{
			V_strncpy(error, "Failed to create trampoline for CCSPlayer_MovementServices::CheckJumpPre", maxlen);
	
			return false;
		}
	
		if (!upCCSPlayer_MovementServices_CheckJumpPre->Install())
		{
			V_strncpy(error, "Failed to install hook for CCSPlayer_MovementServices::CheckJumpPre", maxlen);
	
			return false;
		}
	}

	m_pCCSPlayer_MovementServices_CheckJumpPre = upCCSPlayer_MovementServices_CheckJumpPre.release();
	sv_autobunnyhopping = g_pCVar->GetConVar(g_pCVar->FindConVar("sv_autobunnyhopping"));
	g_pCVar->InstallGlobalChangeCallback(OnConVarChanged);

	g_SMAPI->AddListener(this, this);

	return true;
}

bool VIPBhop::Unload(char *error, size_t maxlen)
{
	delete m_pCCSPlayer_MovementServices_CheckJumpPre;
	g_pCVar->RemoveGlobalChangeCallback(OnConVarChanged);
	
	return true;
}

void VIPBhop::AllPluginsLoaded()
{
	int ret;
	g_pVIPCore = static_cast<IVIPApi*>(g_SMAPI->MetaFactory(VIP_INTERFACE, &ret, nullptr));

	if (ret != META_IFACE_OK)
	{
		Warning("[%s] Could not find cs2-vip\n", GetLogTag());

		return;
	}

	g_pVIPCore->VIP_OnVIPLoaded([]()
	{
		g_pVIPCore->VIP_RegisterFeature(g_szFeature, VIP_BOOL, TOGGLABLE, nullptr, OnBhopToggle);
		g_pGameEntitySystem = g_pVIPCore->VIP_GetEntitySystem();
	});

	g_pVIPCore->VIP_OnClientLoaded(OnClientLoaded);
	g_pVIPCore->VIP_OnClientDisconnect(OnClientDisconnect);
	g_pVIPCore->VIP_OnVIPClientAdded(OnVIPClientAdded);
	g_pVIPCore->VIP_OnVIPClientRemoved(OnVIPClientRemoved);
}

void OnConVarChanged(ConVarRefAbstract* cvar, CSplitScreenSlot nSlot, const char* pNewValue, const char* pOldValue)
{
	if (cvar->m_pConVarState == sv_autobunnyhopping && !*reinterpret_cast<bool*>(&sv_autobunnyhopping->values))
	{
		for (int i = 0; i < std::size(g_bBhop); i++)
		{
			if (g_bBhop[i])
			{
				SetPlayerConVar(i, "sv_autobunnyhopping", "true");
			}
		}
	}
}

bool VIPBhop::OnBhopToggle(int iSlot, const char* szFeature, VIP_ToggleState eOldStatus, VIP_ToggleState& eNewStatus)
{
	g_bBhop[iSlot] = eNewStatus == VIP_ToggleState::ENABLED;

	if (!*reinterpret_cast<bool*>(&sv_autobunnyhopping->values))
	{
		SetPlayerConVar(iSlot, "sv_autobunnyhopping", g_bBhop[iSlot] ? "true" : "false");
	}

	return false;
}

void VIPBhop::OnClientLoaded(int iSlot, bool bIsVIP)
{
	if (bIsVIP && g_pVIPCore->VIP_GetClientFeatureBool(iSlot, g_szFeature))
	{
		SetPlayerConVar(iSlot, "sv_autobunnyhopping", "true");

		g_bBhop[iSlot] = true;
	}
}

void VIPBhop::OnClientDisconnect(int iSlot, bool bIsVIP)
{
	g_bBhop[iSlot] = false;
}

void VIPBhop::OnVIPClientAdded(int iSlot)
{
	if (g_pVIPCore->VIP_GetClientFeatureBool(iSlot, g_szFeature))
	{
		SetPlayerConVar(iSlot, "sv_autobunnyhopping", "true");

		g_bBhop[iSlot] = true;
	}
}

void VIPBhop::OnVIPClientRemoved(int iSlot, int iReason)
{
	if (g_bBhop[iSlot])
	{
		if (!*reinterpret_cast<bool*>(&sv_autobunnyhopping->values))
		{
			SetPlayerConVar(iSlot, "sv_autobunnyhopping", "false");
		}

		g_bBhop[iSlot] = false;
	}
}

void CCSPlayer_MovementServices_CheckJumpPre(CCSPlayer_MovementServices* _this, void* mv)
{
	bool& autobunnyhopping = *reinterpret_cast<bool*>(&sv_autobunnyhopping->values);

	if (!autobunnyhopping && g_bBhop[_this->m_pPawn->m_hController().GetEntryIndex() - 1])
	{
		autobunnyhopping = true;

		vCCSPlayer_MovementServices_CheckJumpPre(_this, mv);

		autobunnyhopping = false;

		return;
	}
	
	vCCSPlayer_MovementServices_CheckJumpPre(_this, mv);
}

void SetPlayerConVar(CPlayerSlot nSlot, const char* name, const char* value)
{
	INetChannel* pNetChannel = reinterpret_cast<INetChannel*>(engine->GetPlayerNetInfo(nSlot));

	if (pNetChannel)
	{
		static INetworkSerializable* pCNETMsg_SetConVar = g_pNetworkMessages->FindNetworkMessagePartial("CNETMsg_SetConVar");

		CNETMsg_SetConVar msg;
		auto cvar = msg.mutable_convars()->add_cvars();
		cvar->set_name(name);
		cvar->set_value(value);
	
		pNetChannel->SendNetMessage(pCNETMsg_SetConVar, &msg, BUF_DEFAULT);
	}
}

CGameEntitySystem* GameEntitySystem()
{
	return g_pGameEntitySystem;
}

///////////////////////////////////////
const char* VIPBhop::GetLicense()
{
	return "GPL";
}

const char* VIPBhop::GetVersion()
{
	return "1.0.0";
}

const char* VIPBhop::GetDate()
{
	return __DATE__;
}

const char* VIPBhop::GetLogTag()
{
	return "VIPBhop";
}

const char* VIPBhop::GetAuthor()
{
	return u8"Phoenix (˙·٠●Феникс●٠·˙)";
}

const char* VIPBhop::GetDescription()
{
	return "VIP Bhop";
}

const char* VIPBhop::GetName()
{
	return "VIP Bhop";
}

const char* VIPBhop::GetURL()
{
	return "https://github.com/komashchenko/VIPBhop";
}
