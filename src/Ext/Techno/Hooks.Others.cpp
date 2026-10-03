#include "Body.h"

#include <EventClass.h>
#include <SpawnManagerClass.h>
#include <OverlayClass.h>
#include <TerrainClass.h>
#include <JumpjetLocomotionClass.h>
#include <Kamikaze.h>

#include <Ext/Anim/Body.h>
#include <Ext/Aircraft/Body.h>
#include <Ext/Building/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/OverlayType/Body.h>
#include <Ext/TerrainType/Body.h>
#include <Ext/Scenario/Body.h>
#include <Ext/Unit/Body.h>
#include <Utilities/AresHelper.h>
#include <Utilities/Helpers.Alex.h>
#include <Helpers/Macro.h>

#include <TacticalClass.h>
#include <Commands/FrameByFrame.h>

#pragma region AttackWall

DEFINE_HOOK(0x6F8C18, TechnoClass_ScanToAttackWall_PlayerDestroyWall, 0x6)
{
	enum { SkipIsAIChecks = 0x6F8C52, FuncRetZero = 0x6F8DE3 };

	GET(TechnoClass*, pThis, ESI);

	if (!pThis->Owner->IsControlledByHuman())
		return 0;

	return RulesExt::Global()->PlayerDestroyWalls ? SkipIsAIChecks : FuncRetZero;
}

DEFINE_HOOK(0x6F8D21, TechnoClass_ScanToAttackWall_CheckWH, 0x6)
{
	GET(WarheadTypeClass*, pWH, ECX);

	bool result = pWH->Wall;

	if (result)
	{
		bool defaultValue = false;

		if (RulesExt::Global()->AutoTargetWalls > 0)
			defaultValue = true;
		else if (RulesExt::Global()->AutoTargetWalls < 0)
			defaultValue = pWH->WallAbsoluteDestroyer;

		result = WarheadTypeExt::Fetch(pWH)->AutoTargetWalls.Get(defaultValue);
	}

	R->AL(result);
	return R->Origin() + 0x6;
}

DEFINE_HOOK(0x6F8D32, TechnoClass_ScanToAttackWall_DestroyOwnerlessWalls, 0x9)
{
	enum { GoOtherChecks = 0x6F8D58, NotOkToFire = 0x6F8DE3 };

	GET(int, OwnerIdx, EAX);
	GET(TechnoClass*, pThis, ESI);

	if (auto const pOwner = (OwnerIdx != -1) ? HouseClass::Array.Items[OwnerIdx] : nullptr)
	{
		if (pOwner->IsAlliedWith(pThis->Owner)
			&& (!RulesExt::Global()->DestroyOwnerlessWalls
			|| !pOwner->IsNeutral()))
		{
			return NotOkToFire;
		}
	}

	return GoOtherChecks;
}

DEFINE_HOOK(0x6F9B1B, TechnoClass_SelectAutoTarget_EndAutoTargetingIfFindWalls1, 0x5)
{
	enum { SkipGameCode = 0x6F9B37 };
	return RulesExt::Global()->EndAutoTargetingIfFindWalls ? 0 : SkipGameCode;
}

DEFINE_HOOK(0x6F9B3E, TechnoClass_SelectAutoTarget_EndAutoTargetingIfFindWalls2, 0x6)
{
	enum { CheckNext = 0x6F9B44 , TargetTechno = 0x6F9DA1 , TargetWall = 0x6F9B55 };

	GET(int, maxRange, ECX);
	GET(int, currentRange, EDI);
	GET_STACK(AbstractClass*, pBestTarget, STACK_OFFSET(0x6C, -0x4C));
	GET_STACK(CellStruct, bestTargetCell, STACK_OFFSET(0x6C, -0x38));

	if (currentRange > maxRange)
		return RulesExt::Global()->EndAutoTargetingIfFindWalls || pBestTarget || bestTargetCell == CellStruct::Empty ? TargetTechno : TargetWall;

	return CheckNext;
}

DEFINE_HOOK(0x6F9B64, TechnoClass_SelectAutoTarget_RecordAttackWall, 0x7)
{
	GET(TechnoClass*, pThis, ESI);
	GET(CellClass*, pCell, EAX);

	TechnoExt::Fetch(pThis)->AutoTargetedWallCell = pCell;
	return 0;
}

#pragma endregion
