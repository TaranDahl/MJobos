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

#pragma region FixRepairDistance

DEFINE_PATCH(0x44C70B, 0xC8);
DEFINE_JUMP(LJMP, 0x44C75E, 0x44C793);

#pragma endregion

#pragma region HealingWeaponFix

// Skip the hardcode of healing weapon auto target range.
DEFINE_JUMP(LJMP, 0x6F9024, 0x6F9042);

DEFINE_HOOK(0x6FA9D8, TechnoClass_Update_FixRepairWeapon, 0x6)
{
	enum { SkipGameCode = 0x6FAA6F };

	GET(TechnoClass*, pThis, ESI);

	if (pThis->Target && pThis->CombatDamage(-1) < 0)
	{
		if ((SessionClass::IsCampaign() ? pThis->Owner->IsControlledByCurrentPlayer() : pThis->Owner->IsHumanPlayer) && !pThis->Owner->IsAlliedWith(pThis->Target))
			pThis->SetTarget(nullptr);
		else if (pThis->CurrentMission == Mission::Guard && !pThis->IsCloseEnoughToAttack(pThis->Target))
			pThis->SetTarget(nullptr);
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x707ED0, TechnoClass_GetGuardRange_FixForIFV, 0x6)
{
	enum { SkipGameCode = 0x707F08 };

	GET(TechnoClass*, pThis, ESI);

	auto pType = pThis->GetTechnoType();

	if (!pType->HasMultipleTurrets() || pType->IsGattling)
		return 0;

	R->EAX(pThis->GetWeaponRange(pThis->CurrentWeaponNumber));
	return SkipGameCode;
}

#pragma endregion

#pragma region AIAdjacentMax

DEFINE_HOOK_AGAIN(0x42EB6A, BaseClass_GetBaseNodeIndex_AIAdjacentMax, 0x8);
DEFINE_HOOK(0x42EBA2, BaseClass_GetBaseNodeIndex_AIAdjacentMax, 0x8)
{
	GET(BaseClass*, pThis, ESI);
	GET(const int, nodeIdx, EDI);

	bool isValid = reinterpret_cast<bool(__thiscall*)(BaseClass*, int)>(0x42E780)(pThis, nodeIdx);
	const int rangeLimit = SessionClass::Instance.IsCampaign()
		? RulesExt::Global()->AIAdjacentMax_Campaign.Get(RulesExt::Global()->AIAdjacentMax)
		: RulesExt::Global()->AIAdjacentMax;

	if (rangeLimit >= 0 && isValid)
	{
		const auto node = pThis->BaseNodes[nodeIdx];
		const auto pOwner = pThis->Owner;
		const auto pBuildingType = BuildingTypeClass::Array[node.BuildingTypeIndex];
		const CellStruct offset
		{
			static_cast<short>(pBuildingType->GetFoundationWidth() / 2),
			static_cast<short>(pBuildingType->GetFoundationHeight(false) / 2)
		};
		const auto center = node.MapCoords + offset;
		const auto cellList = GeneralUtils::AdjacentCellsInRange(rangeLimit);
		bool hasAdjacent = false;

		for (const auto& cell : cellList)
		{
			const auto pBuilding = MapClass::Instance.GetCellAt(cell + center)->GetBuilding();

			if (!pBuilding || pBuilding->Owner != pOwner)
				continue;

			const auto pType = pBuilding->Type;
			const auto baseNormalDefault = (!pType->UndeploysInto || !pType->ResourceGatherer) && !pBuilding->IsStrange();

			if (BuildingTypeExt::Fetch(pType)->AIBaseNormal.Get(baseNormalDefault))
			{
				hasAdjacent = true;
				break;
			}
		}

		isValid = hasAdjacent;
	}

	R->AL(isValid);
	return R->Origin() + 0x8;
}

#pragma endregion

#pragma region AIProtectBase

DEFINE_HOOK(0x70821F, TechnoClass_BaseIsAttacked_Ignore1, 0x6)
{
	enum { CheckDefend = 0x70822B, SkipDefend = 0x7083BC };
	GET(TeamClass*, pTeam, EAX);
	GET(bool, isBaseDefense, ECX);
	GET(FootClass*, pThis, ESI);

	if (isBaseDefense)
		return CheckDefend;

	if (TechnoTypeExt::Fetch(pThis->GetTechnoType())->AIDefendBase_Ignore)
		return SkipDefend;

	return pTeam ? SkipDefend : CheckDefend;
}

DEFINE_HOOK(0x708455, TechnoClass_BaseIsAttacked_Ignore2, 0x6)
{
	enum { CheckDefend = 0x708461, SkipDefend = 0x708622 };
	GET(TeamClass*, pTeam, EAX);
	GET(bool, isBaseDefense, ECX);
	GET(FootClass*, pThis, ESI);

	if (isBaseDefense)
		return CheckDefend;

	if (TechnoTypeExt::Fetch(pThis->GetTechnoType())->AIDefendBase_Ignore)
		return SkipDefend;

	return pTeam ? SkipDefend : CheckDefend;
}

#pragma endregion
