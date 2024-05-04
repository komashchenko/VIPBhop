#pragma once
#include "CBaseCombatCharacter.h"
#include "CBasePlayerController.h"
#include "ehandle.h"
#include "schemasystem_helper.h"

class CBasePlayerPawn : public CBaseCombatCharacter
{
public:
	SCHEMA_FIELD(CHandle<CBasePlayerController>, CBasePlayerPawn, m_hController);
};