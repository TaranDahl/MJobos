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
