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
#pragma region VHPScan

// 具有VHPScan=Strong的单位目标死亡时
static inline void CheckVHPScanAndRetarget(TechnoClass* pThis)
{
	if (!RulesExt::Global()->VHPScan_Enhanced)
		return;

	const auto pType = pThis->GetTechnoType();

	if (pType->VHPScan != 2)
		return;

	const auto pTargetTechno = abstract_cast<TechnoClass*>(pThis->Target);

	if (!pTargetTechno || pTargetTechno->EstimatedHealth > 0 || pThis->Owner->IsAlliedWith(pTargetTechno))
		return;

	pThis->SetTarget(nullptr);

}

DEFINE_HOOK(0x5206B7, InfantryClass_UpdateFiring_Start, 0x6)
{
	GET(InfantryClass*, pThis, EBP);
	CheckVHPScanAndRetarget(pThis);
	return 0;
}

DEFINE_HOOK(0x736DF8, UnitClass_UpdateFiring_Start, 0x6)
{
	GET(UnitClass*, pThis, ESI);
	CheckVHPScanAndRetarget(pThis);
	return 0;
}

DEFINE_HOOK(0x44ACF0, BuildingClass_MissionAttack_Start, 0x6)
{
	GET(BuildingClass*, pThis, ECX);
	CheckVHPScanAndRetarget(pThis);
	return 0;
}

DEFINE_HOOK(0x417FF1, AircraftClass_MissionAttack_Start, 0x6)
{
	GET(AircraftClass*, pThis, ESI);
	CheckVHPScanAndRetarget(pThis);
	return 0;
}

DEFINE_HOOK(0x6F7D0D, TechnoClass_CanAutoTargetObject_VHPScanStrong, 0x6)
{
	enum { SkipGameCode = 0x6F7D19 };
	return RulesExt::Global()->VHPScan_Enhanced ? SkipGameCode : 0;
}

DEFINE_HOOK(0x6F8721, TechnoClass_CanAutoTargetObject_VHPScanThreat, 0x7)
{
	enum { SkipGameCode = 0x6F875F };

	GET(TechnoClass*, pThis, EDI);
	GET(ObjectClass*, pTarget, ESI);
	GET_STACK(int*, pThreat, STACK_OFFSET(0x3C, 0x14));

	if (RulesExt::Global()->VHPScan_Enhanced && pThis->GetTechnoType()->VHPScan == 2)
	{
		if (pTarget->EstimatedHealth <= 0)
			*pThreat /= 10;

		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x6F9F7B, TechnoClass_Update_EstimateHealth, 0x7)
{
	enum { SkipGameCode = 0x6F9F9F };

	if (!RulesExt::Global()->VHPScan_Enhanced)
		return 0;

	GET(TechnoClass*, pThis, ESI);

	if (pThis->EstimatedHealth < pThis->Health && !TechnoExt::Fetch(pThis)->BulletsTargetingMeCount)
		pThis->EstimatedHealth = pThis->Health;

	return SkipGameCode;
}

#pragma endregion

#pragma region Decloak

DEFINE_HOOK(0x6FBC74, TechnoClass_UpdateCloak_LowHealth, 0x6)
{
	return RulesExt::Global()->Decloak_OnCloakingWithLowHealth ? 0 : R->Origin() + 0xC;
}

#pragma endregion
