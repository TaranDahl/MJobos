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

#pragma region GuardRange

DEFINE_HOOK(0x4D6E83, FootClass_MissionAreaGuard_FollowStray, 0x6)
{
	enum { SkipGameCode = 0x4D6E8F };

	GET(FootClass* const, pThis, ESI);

	int range = RulesClass::Instance->GuardModeStray;

	if (const auto pTypeExt = TechnoExt::Fetch(pThis)->TypeExtData)
		range = pThis->Owner->IsControlledByHuman() ? pTypeExt->PlayerGuardModeStray.Get(Leptons(range)) : pTypeExt->AIGuardModeStray.Get(Leptons(range));

	R->EDI(range);
	return SkipGameCode;
}

DEFINE_HOOK(0x4D6E97, FootClass_MissionAreaGuard_Pursuit, 0x6)
{
	enum { KeepTarget = 0x4D6ED1, RemoveTarget = 0x4D6EB3 };

	GET(FootClass* const, pThis, ESI);
	GET(int, range, EDI);
	GET(AbstractClass* const, pFocus, EAX);

	const auto pTypeExt = TechnoExt::Fetch(pThis)->TypeExtData;
	const bool isPlayer = pThis->Owner->IsControlledByHuman();

	if ((pFocus->AbstractFlags & AbstractFlags::Foot) == AbstractFlags::None)
	{
		const Leptons stationaryStray = isPlayer ? pTypeExt->PlayerGuardStationaryStray.Get(RulesExt::Global()->PlayerGuardStationaryStray) : pTypeExt->AIGuardStationaryStray.Get(RulesExt::Global()->AIGuardStationaryStray);

		if (stationaryStray != Leptons(-256))
			range = stationaryStray;
	}

	return ((!(isPlayer ? pTypeExt->PlayerGuardModePursuit.Get(RulesExt::Global()->PlayerGuardModePursuit) : pTypeExt->AIGuardModePursuit.Get(RulesExt::Global()->AIGuardModePursuit))
			|| (!pThis->IsFiring && !pThis->Destination))
		&& pThis->DistanceFrom(pFocus) > range)
		? RemoveTarget
		: KeepTarget;
}

DEFINE_HOOK(0x707F08, TechnoClass_GetGuardRange_AreaGuardRange, 0x5)
{
	enum { SkipGameCode = 0x707E70 };

	GET(Leptons, guardRange, EAX);
	GET(int, mode, EDI);
	GET(TechnoClass* const, pThis, ESI);

	const bool isPlayer = pThis->Owner->IsControlledByHuman();
	const auto pRulesExt = RulesExt::Global();
	const auto pTypeExt = TechnoExt::Fetch(pThis)->TypeExtData;

	const auto& [multiplier, addend, max] = isPlayer
		? std::make_tuple(pTypeExt->PlayerGuardModeGuardRangeMultiplier.Get(pRulesExt->PlayerGuardModeGuardRangeMultiplier), pTypeExt->PlayerGuardModeGuardRangeAddend.Get(pRulesExt->PlayerGuardModeGuardRangeAddend), pRulesExt->PlayerGuardModeGuardRangeMax.Get())
		: std::make_tuple(pTypeExt->AIGuardModeGuardRangeMultiplier.Get(pRulesExt->AIGuardModeGuardRangeMultiplier), pTypeExt->AIGuardModeGuardRangeAddend.Get(pRulesExt->AIGuardModeGuardRangeAddend), pRulesExt->AIGuardModeGuardRangeMax.Get());

	const Leptons min = Leptons((mode == 2) ? (7 / Unsorted::LeptonsPerCell) : 0);
	const Leptons areaGuardRange = Leptons(static_cast<int>(static_cast<int>(guardRange) * multiplier + static_cast<int>(addend)));

	R->EAX(Math::clamp(areaGuardRange, min, max));

	return SkipGameCode;
}

#pragma endregion
