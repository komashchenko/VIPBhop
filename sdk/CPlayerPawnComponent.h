#pragma once
#include "schemasystem_helper.h"
#include "CBasePlayerPawn.h"

class CPlayerPawnComponent
{
public:
	virtual ~CPlayerPawnComponent() = 0;
private:
	[[maybe_unused]] uint8_t __pad0008[0x28]; // 0x8
public:
	CBasePlayerPawn* m_pPawn; // 0x30
};