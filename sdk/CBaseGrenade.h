#pragma once
#include "CBaseFlex.h"
#include "CCSPlayerPawn.h"
#include "schemasystem_helper.h"

class CBaseGrenade : public CBaseFlex
{
public:
	SCHEMA_FIELD(CHandle<CCSPlayerPawn>, CBaseGrenade, m_hThrower);
};