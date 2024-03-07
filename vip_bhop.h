﻿/**
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

#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

class CGameEntitySystem;

#include <ISmmPlugin.h>
#include <sh_vector.h>
#include "vip.h"
#include "subhook.h"

class VIPBhop final : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late);
	bool Unload(char* error, size_t maxlen);
	
private:
	const char* GetAuthor();
	const char* GetName();
	const char* GetDescription();
	const char* GetURL();
	const char* GetLicense();
	const char* GetVersion();
	const char* GetDate();
	const char* GetLogTag();

private: // Hooks
	void AllPluginsLoaded();

	subhook::Hook* m_pCCSPlayer_MovementServices_CheckJumpPre;

private: // VIP
	static bool OnBhopToggle(int iSlot, const char* szFeature, VIP_ToggleState eOldStatus, VIP_ToggleState& eNewStatus);
	static void OnClientLoaded(int iSlot, bool bIsVIP);
	static void OnClientDisconnect(int iSlot, bool bIsVIP);
	static void OnVIPClientAdded(int iSlot);
	static void OnVIPClientRemoved(int iSlot, int iReason);
};

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
