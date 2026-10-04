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

#pragma region ExtendedStray

bool IsCloseToCenter(TechnoClass* pMember, CellClass* pCenterCell, int stray)
{
	// Vanilla check
	if (pMember->DistanceFrom3D(pCenterCell) <= stray)
		return true;

	auto GetOccupiedCount = [](TechnoClass* pTechno) -> int
		{
			switch (pTechno->WhatAmI())
			{
			case AbstractType::Building:
			{
				auto pBuildingType = ((BuildingClass*)pTechno)->Type;
				if (BuildingTypeExt::Fetch(pBuildingType)->IsPassable)
					return 0;

				int cellCount = 0;
				for (auto pFoundation = pBuildingType->GetFoundationData(false); *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
					cellCount += 3;
				return cellCount;
			}
			case AbstractType::Unit:
			case AbstractType::Aircraft:
				return 3;
			case AbstractType::Infantry:
				return 1;
			default:
				return 3;
			}
		};

	auto isAreaFull = [&](int stray) -> bool
		{
			// 距离中心的可用距离
			double distInCell = (double)stray / 256;

			// 大概估计有多少个格子可用, 对角线长为2倍stray的正方形
			int inRangeCellCount = (int)(distInCell * distInCell * 2);

			// 大概估计有多少个位置被占用, 一个格子按3个位置算 , 步兵站1个, 载具占3个
			int inRangeTechnoCount = 0;
			auto crd = pCenterCell->GetCoords();
			for (auto const pTarget : Helpers::Alex::getCellSpreadItems(crd, distInCell))
				inRangeTechnoCount += GetOccupiedCount(pTarget);

			return inRangeTechnoCount >= inRangeCellCount * 3;
		};

	// 看stray范围是否塞满
	if (!isAreaFull(stray))
		return false;

	// 看当前位置到中心位置距离是否塞满
	return isAreaFull(pMember->DistanceFrom(pCenterCell));
}

DEFINE_HOOK(0x6EB680, TeamClass_ProcessAttack_Check, 0x5)
{
	if (!RulesExt::Global()->ExtendedStray)
		return 0;

	enum { CloseToCenter = 0x6EB6C5 };

	GET(FootClass*, pMember, ESI);
	GET(TeamClass*, pThis, EBP);
	GET(int, stray, EDI);

	return IsCloseToCenter(pMember, pThis->SpawnCell, stray) ? CloseToCenter : R->Origin() + 0xF;
}

DEFINE_HOOK(0x6EBB86, TeamClass_ProcessMove_Check, 0x6)
{
	if (!RulesExt::Global()->ExtendedStray)
		return 0;

	enum { CloseToCenter = 0x6EBC8D };

	GET(FootClass*, pMember, ESI);
	GET(TeamClass*, pThis, EBP);
	GET(int, stray, EDI);

	return IsCloseToCenter(pMember, pThis->SpawnCell, stray) ? CloseToCenter : R->Origin() + 0x13;
}

DEFINE_HOOK(0x6EBF2F, TeamClass_ProcessMove_AllMemberArrived, 0x6)
{
	if (!RulesExt::Global()->ExtendedStray)
		return 0;

	enum { Arrived = 0x6EBF37, NotArrived = 0x6EBF45 };

	GET(TeamClass*, pThis, EBP);
	ScriptActionNode buffer;
	auto currentAction = pThis->CurrentScript->GetCurrentAction(&buffer)->Action;
	int stray = currentAction == 54 || currentAction == 53 ? RulesClass::Instance->RelaxedStray : RulesClass::Instance->Stray;
	return pThis->SpawnCell && pThis->SpawnCell->DistanceFrom3D(pThis->Focus) <= stray ? Arrived : NotArrived;
}

#pragma endregion
