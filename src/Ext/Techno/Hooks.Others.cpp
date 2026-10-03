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

#pragma region EngineerAutoFire

DEFINE_HOOK(0x707E84, TechnoClass_GetGuardRange_Engineer, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	R->AL(pThis->IsEngineer() && !TechnoExt::Fetch(pThis)->TypeExtData->Engineer_CanAutoFire);
	return 0;
}

DEFINE_HOOK(0x6F8EF1, TechnoClass_SelectAutoTarget_Engineer, 0x6)
{
	enum { SkipGameCode = 0x6F8EF7 };

	GET(InfantryTypeClass* const, pType, EAX);

	R->CL(pType->Engineer && !TechnoTypeExt::Fetch(pType)->Engineer_CanAutoFire);
	return SkipGameCode;
}

DEFINE_HOOK(0x709249, TechnoClass_CanPassiveAcquireNow_Engineer1, 0xA)
{
	enum { SkipGameCode = 0x709253 };

	GET(TechnoClass* const, pThis, ESI);

	R->AL(pThis->IsEngineer() && !TechnoExt::Fetch(pThis)->TypeExtData->Engineer_CanAutoFire);
	return SkipGameCode;
}

DEFINE_HOOK(0x6F8AEC, TechnoClass_TryAutoTargetObject_Engineer1, 0x6)
{
	enum { SkipGameCode = 0x6F8AF2 };

	GET(TechnoClass* const, pThis, ESI);

	R->AL(pThis->IsEngineer() && !TechnoExt::Fetch(pThis)->TypeExtData->Engineer_CanAutoFire);
	return SkipGameCode;
}

DEFINE_HOOK(0x6F8BB2, TechnoClass_TryAutoTargetObject_Engineer2, 0x6)
{
	enum { SkipGameCode = 0x6F8BB8 };

	GET(TechnoClass* const, pThis, ESI);

	R->AL(pThis->IsEngineer() && !TechnoExt::Fetch(pThis)->TypeExtData->Engineer_CanAutoFire);
	return SkipGameCode;
}

#pragma endregion
