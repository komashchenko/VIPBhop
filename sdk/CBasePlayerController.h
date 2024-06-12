#pragma once
#include "CBaseEntity.h"
#include "CBasePlayerPawn.h"
#include "ehandle.h"
#include "schemasystem_helper.h"

class CBasePlayerController : public CBaseEntity
{
public:
	SCHEMA_FIELD(CHandle<CBasePlayerPawn>, CBasePlayerController, m_hPawn);
};