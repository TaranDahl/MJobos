#include "Body.h"
#include "ProximityTemp.h"

#include <Ext/Rules/Body.h>
#include <Ext/Scenario/Body.h>
#include <Ext/TerrainType/Body.h>
#include <Utilities/GeneralUtils.h>

static inline void PlayConstructionYardAnim(BuildingClass* pFactory)
{
	const auto pFactoryType = pFactory->Type;

	if (!pFactoryType->ConstructionYard)
		return;

	VocClass::PlayGlobal(RulesClass::Instance->BuildingSlam, 0x2000, 1.0);
	pFactory->DestroyNthAnim(BuildingAnimSlot::PreProduction);
	pFactory->DestroyNthAnim(BuildingAnimSlot::Idle);

	const bool damaged = pFactory->GetHealthPercentage() <= RulesClass::Instance->ConditionYellow;
	const auto pAnimName = damaged ? pFactoryType->BuildingAnim[8].Damaged : pFactoryType->BuildingAnim[8].Anim;

	if (pAnimName && *pAnimName)
		pFactory->PlayAnim(pAnimName, BuildingAnimSlot::Production, damaged, false);
}

DEFINE_HOOK(0x4FB1EA, HouseClass_UnitFromFactory_LimboBuild, 0x5)
{
	enum { BuildSucceeded = 0x4FB649 };

	GET(HouseClass* const, pHouse, EBP);
	GET(TechnoClass* const, pTechno, ESI);
	GET(BuildingClass* const, pFactory, EDI);

	if (pTechno->WhatAmI() != AbstractType::Building)
		return 0;

	auto const pBuilding = static_cast<BuildingClass*>(pTechno);
	const auto pTypeExt = BuildingTypeExt::Fetch(pBuilding->Type);

	if (!pTypeExt->LimboBuild)
		return 0;

	BuildingTypeExt::CreateLimboBuilding(pBuilding, pBuilding->Type, pHouse, pTypeExt->LimboBuildID);

	auto& display = DisplayClass::Instance;

	if (display.CurrentBuilding == pBuilding && HouseClass::CurrentPlayer == pHouse)
	{
		display.SetActiveFoundation(nullptr);
		display.CurrentBuilding = nullptr;
		display.CurrentBuildingType = nullptr;
	}

	PlayConstructionYardAnim(pFactory);
	return BuildSucceeded;
}

// AIConstructionYard Hook #1 -> sub_740810 - Check number of construction yard before deploy.
DEFINE_HOOK(0x740A11, UnitClass_Mission_Guard_AIAutoDeployMCV, 0x6)
{
	enum { SkipGameCode = 0x740A50 };

	GET(UnitClass*, pMCV, ESI);

	return (!RulesExt::Global()->AIAutoDeployMCV && pMCV->Owner->NumConYards > 0) ? SkipGameCode : 0;
}

// AIConstructionYard Hook #2 -> sub_7393C0 - Skip useless base center setting.
DEFINE_HOOK(0x739889, UnitClass_TryToDeploy_AISetBaseCenter, 0x6)
{
	enum { SkipGameCode = 0x73992B };

	GET(UnitClass*, pMCV, EBP);

	return (!RulesExt::Global()->AISetBaseCenter && pMCV->Owner->NumConYards > 1) ? SkipGameCode : 0;
}

// AIConstructionYard Hook #3 -> sub_4FD500 - Update better base center.
DEFINE_HOOK(0x4FD538, HouseClass_AIHouseUpdate_CheckAIBaseCenter, 0x7)
{
	if (RulesExt::Global()->AIBiasSpawnCell && !SessionClass::IsCampaign())
	{
		GET(HouseClass*, pAI, EBX);

		if (pAI->ConYards.Count > 1)
		{
			const int wayPoint = pAI->GetSpawnPosition();

			if (wayPoint != -1)
			{
				const auto center = ScenarioClass::Instance->GetWaypointCoords(wayPoint);
				auto newCenter = center;
				double distanceSquared = 131072.0;

				for (int i = 0; i < pAI->ConYards.Count; ++i)
				{
					if (const auto pBuilding = pAI->ConYards.GetItem(i))
					{
						if (pBuilding->IsAlive && pBuilding->Health > 0 && !pBuilding->InLimbo)
						{
							const double newDistanceSquared = pBuilding->GetMapCoords().DistanceFromSquared(center);

							if (newDistanceSquared < distanceSquared)
							{
								distanceSquared = newDistanceSquared;
								newCenter = pBuilding->GetMapCoords();
							}
						}
					}
				}

				if (newCenter != center)
				{
					pAI->BaseSpawnCell = newCenter;
					pAI->Base.Center = newCenter;
				}
			}
		}
	}

	return 0;
}

// Buildable-upon TerrainTypes Hook #3 - Remove them when buildings are placed on them.
DEFINE_HOOK(0x5684B1, MapClass_PlaceDown_BuildableUponTypes, 0x6)
{
	GET(ObjectClass*, pObject, EDI);
	GET(CellClass*, pCell, EAX);

	if (pObject->WhatAmI() == AbstractType::Building)
	{
		if (auto const pTerrain = pCell->GetTerrain(false))
		{
			auto const pType = pTerrain->Type;
			auto const pTypeExt = TerrainTypeExt::Fetch(pType);
			bool const canBuild = pType->SpawnsTiberium
				? pTypeExt->CanBeBuiltOn.Get(RulesExt::Global()->Tibtree_CanBeBuiltOn)
				: pTypeExt->CanBeBuiltOn.Get(RulesExt::Global()->Terrain_CanBeBuiltOn);

			if (canBuild)
			{
				pCell->RemoveContent(pTerrain, false);
				TerrainTypeExt::Remove(pTerrain);
			}
		}
	}

	return 0;
}

// Buildable-upon TerrainTypes Hook #4 -> Allow placing walls on top of terrain
DEFINE_HOOK(0x5FD2B6, OverlayClass_Unlimbo_SkipTerrainCheck, 0x9)
{
	enum { Unlimbo = 0x5FD2CA, NoUnlimbo = 0x5FD2C3 };

	GET(CellClass* const, pCell, EAX);

	if (!Game::IsActive)
		return Unlimbo;

	if (auto const pTerrain = pCell->GetTerrain(false))
	{
		auto const pType = pTerrain->Type;
		auto const pTypeExt = TerrainTypeExt::Fetch(pType);
		bool const canBuild = pType->SpawnsTiberium
			? pTypeExt->CanBeBuiltOn.Get(RulesExt::Global()->Tibtree_CanBeBuiltOn)
			: pTypeExt->CanBeBuiltOn.Get(RulesExt::Global()->Terrain_CanBeBuiltOn);

		if (!canBuild)
			return NoUnlimbo;

		pCell->RemoveContent(pTerrain, false);
		TerrainTypeExt::Remove(pTerrain);
	}

	return Unlimbo;
}

// Buildable-upon TerrainTypes Hook #5 -> Ignore when flushing building foundations for placement.
DEFINE_HOOK(0x45EF3A, BuildingTypeClass_FlushForPlacement_BuildableTerrain, 0x7)
{
	enum { Disallow = 0x45F00B, Continue = 0x45EF4A };

	GET(ObjectClass* const, pObject, ESI);

	if (auto const pTerrain = abstract_cast<TerrainClass*>(pObject))
	{
		auto const pType = pTerrain->Type;
		auto const pTypeExt = TerrainTypeExt::Fetch(pType);
		bool const canBuild = pType->SpawnsTiberium
			? pTypeExt->CanBeBuiltOn.Get(RulesExt::Global()->Tibtree_CanBeBuiltOn)
			: pTypeExt->CanBeBuiltOn.Get(RulesExt::Global()->Terrain_CanBeBuiltOn);

		if (!canBuild)
			return Disallow;
	}

	return Continue;
}

// Buildable Proximity Helper
namespace ProximityTemp
{
	int DistanceOverride = 0;
	bool SkipDisallowed = false;
	bool Build = false;
	bool Exist = false;
	bool Mouse = false;
	CellClass* CurrentCell = nullptr;
	BuildingTypeClass* BuildType = nullptr;
}

/*
	In sub_4A8EB0

	- BaseNormal extra checking Hook #1-1 -> Set context and clear up data
*/
DEFINE_HOOK(0x4A8F20, DisplayClass_BuildingProximityCheck_SetContext, 0x5)
{
	GET(BuildingTypeClass*, pType, ESI);

	if (RulesExt::Global()->PlacementGrid_Expand)
		BuildingTypeExt::BaseNormalCells.clear();

	ProximityTemp::Build = false;
	ProximityTemp::BuildType = pType;

	return 0;
}

/*
	In sub_4A8EB0

	- BaseNormal extra checking Hook #1-2 -> Check allowed range
*/
DEFINE_HOOK(0x4A8F3E, DisplayClass_BuildingProximityCheck_BeforeChecks, 0x6)
{
	enum { SkipGameCode = 0x4A8F44, ReturnFromFunction = 0x4A904E };

	GET(BuildingTypeClass*, pType, ESI);
	GET_STACK(const int, houseArrayIndex, STACK_OFFSET(0x30, 0x8));
	GET_STACK(CellStruct*, foundationData, STACK_OFFSET(0x30, 0xC));
	GET_STACK(CellStruct*, currentPosition, STACK_OFFSET(0x30, 0x10));

	const auto pTypeExt = BuildingTypeExt::Fetch(pType);
	ProximityTemp::BuildType = pType;
	ProximityTemp::SkipDisallowed = false;

	if (pTypeExt->Adjacent_Disallowed_Prohibit && pTypeExt->Adjacent_Disallowed_ProhibitDistance > 0 && ProximityTemp::DistanceOverride == 0)
	{
		ProximityTemp::DistanceOverride = pTypeExt->Adjacent_Disallowed_ProhibitDistance;
		bool result = DisplayClass::Instance.PassesProximityCheck(pType, houseArrayIndex, foundationData, currentPosition);
		ProximityTemp::DistanceOverride = 0;

		if (!result)
		{
			R->Stack<bool>(STACK_OFFSET(0x30, 0xC), false);
			BuildingTypeExt::BaseNormalCells.clear();
			return ReturnFromFunction;
		}

		ProximityTemp::SkipDisallowed = true;
	}

	int distance = ProximityTemp::DistanceOverride > 0 ? ProximityTemp::DistanceOverride : pType->Adjacent;
	R->EAX(distance);
	return SkipGameCode;
}

/*
	In sub_4A8EB0

	- BaseNormal extra checking Hook #1-3 -> Check allowed building
*/
DEFINE_HOOK(0x4A8FD7, DisplayClass_BuildingProximityCheck_BuildArea, 0x6)
{
	enum { SkipBuilding = 0x4A902C, ReturnFromFunction = 0x4A904E };

	GET(BuildingClass*, pCellBuilding, ESI);
	GET_STACK(const int, houseArrayIndex, STACK_OFFSET(0x30, 0x8));

	auto const pTypeExt = BuildingTypeExt::Fetch(pCellBuilding->Type);

	if (pTypeExt->NoBuildAreaOnBuildup && pCellBuilding->CurrentMission == Mission::Construction)
		return SkipBuilding;

	auto const pTmpTypeExt = BuildingTypeExt::Fetch(ProximityTemp::BuildType);
	auto const& pBuildingsAllowed = pTmpTypeExt->Adjacent_Allowed;

	if (pBuildingsAllowed.size() > 0 && !pBuildingsAllowed.Contains(pCellBuilding->Type))
		return SkipBuilding;

	if (!ProximityTemp::SkipDisallowed && pCellBuilding->Owner->ArrayIndex == houseArrayIndex)
	{
		auto const& pBuildingsDisallowed = pTmpTypeExt->Adjacent_Disallowed;

		if (pBuildingsDisallowed.size() > 0 && pBuildingsDisallowed.Contains(pCellBuilding->Type))
		{
			if (pTmpTypeExt->Adjacent_Disallowed_Prohibit)
			{
				R->Stack<bool>(STACK_OFFSET(0x30, 0xC), false);
				BuildingTypeExt::BaseNormalCells.clear();
				return ReturnFromFunction;
			}
			else
			{
				return SkipBuilding;
			}
		}
	}

	return 0;
}

/*
	In sub_4A8EB0

	- BaseNormal extra checking Hook #1-4 -> Break loop or record cell for drawing
*/
DEFINE_HOOK(0x4A902C, MapClass_PassesProximityCheck_BaseNormalExtra, 0x5)
{
	enum { CheckCompleted = 0x4A904E };

	REF_STACK(bool, canBuild, STACK_OFFSET(0x30, 0xC));

	if (canBuild)
	{
		if (!RulesExt::Global()->PlacementGrid_Expand)
			return CheckCompleted;

		canBuild = false;
		ProximityTemp::Build = true;

		GET_STACK(const CellStruct, currentCell, STACK_OFFSET(0x30, -0x20));
		BuildingTypeExt::BaseNormalCells.push_back(currentCell);
	}

	return 0;
}

/*
	In sub_4A8EB0

	- BaseNormal extra checking Hook #1-5 -> Restore the correct result
*/
DEFINE_HOOK(0x4A904E, MapClass_PassesProximityCheck_RestoreResult, 0x5)
{
	GET_STACK(bool, canBuild, STACK_OFFSET(0x30, 0xC));
	GET_STACK(const int, idxHouse, STACK_OFFSET(0x30, 0x8));

	if (RulesExt::Global()->CheckExtraBaseNormal && !canBuild)
	{
		const auto& baseNormalTechnos = ScenarioExt::Global()->BaseNormalTechnos;

		if (!baseNormalTechnos.empty())
		{
			GET(const int, topLeftX, EBP);
			GET_STACK(const int, foundationWidth, STACK_OFFSET(0x30, -0x1C));
			GET_STACK(const int, topLeftY, STACK_OFFSET(0x30, -0xC));
			GET_STACK(const int, foundationHeight, STACK_OFFSET(0x30, 0x4));

			const auto pBuildType = ProximityTemp::BuildType;
			const auto pBuildTypeExt = BuildingTypeExt::Fetch(pBuildType);
			const auto range = pBuildType->Adjacent + 1;
			const auto maxX = topLeftX + range + foundationWidth;
			const auto maxY = topLeftY + range + foundationHeight;
			const auto minX = topLeftX - range;
			const auto minY = topLeftY - range;

			for (const auto pExt : baseNormalTechnos)
			{
				const auto pTechno = pExt->OwnerObject();

				if (!TechnoExt::IsActive(pTechno))
					continue;

				const auto technoMapCell = pTechno->GetMapCoords();

				if (technoMapCell.X < minX || technoMapCell.Y < minY || technoMapCell.X >= maxX || technoMapCell.Y >= maxY)
					continue;

				const auto pTypeExt = pExt->TypeExtData;
				const auto pOwner = pTechno->Owner;

				if (pOwner->ArrayIndex == idxHouse)
				{
					if (!pTypeExt->ExtraBaseNormal)
						continue;
				}
				else if (!RulesClass::Instance->BuildOffAlly || !pOwner->IsAlliedWith(HouseClass::Array.Items[idxHouse]) || !pTypeExt->ExtraBaseForAllyBuilding)
				{
					continue;
				}

				const auto& extraAllowed = pBuildTypeExt->Adjacent_AllowedExtra;

				if (!extraAllowed.empty() && !extraAllowed.Contains(pTypeExt->OwnerObject()))
					continue;

				const auto& extraDisallowed = pBuildTypeExt->Adjacent_DisallowedExtra;

				if (!extraDisallowed.empty() && extraDisallowed.Contains(pTypeExt->OwnerObject()))
					continue;

				R->Stack<bool>(STACK_OFFSET(0x30, 0xC), true);
				break;
			}
		}
	}
	if (RulesExt::Global()->PlacementGrid_Expand)
		R->Stack<bool>(STACK_OFFSET(0x30, 0xC), ProximityTemp::Build);

	return 0;
}

// Let the game do the proximity and shroud check when the cell which mouse is pointing at has not changed
DEFINE_JUMP(LJMP, 0x4AACD9, 0x4AACF5);
DEFINE_JUMP(LJMP, 0x4A9361, 0x4A9371);

// Record the current grid cell before the game chooses the placing-grid frame.
DEFINE_HOOK(0x47EEBC, CellClass_DrawPlaceGrid_RecordCell, 0x6)
{
	GET(CellClass*, pCell, ESI);
	ProximityTemp::CurrentCell = pCell;
	return 0;
}

// Choose the configured placing-grid frame.
DEFINE_HOOK(0x47EF52, CellClass_DrawPlaceGrid_DrawGrids, 0x6)
{
	const auto pRules = RulesExt::Global();

	if (!pRules->PlacementGrid_Expand)
		return 0;

	const auto pCell = ProximityTemp::CurrentCell;
	const auto cell = pCell->MapCoords;
	const auto pObj = DisplayClass::Instance.CurrentBuildingType;
	const auto range = static_cast<short>((pObj && pObj->WhatAmI() == AbstractType::BuildingType)
		? static_cast<BuildingTypeClass*>(pObj)->Adjacent + 1
		: 0);

	const auto maxX = static_cast<short>(cell.X + range);
	const auto maxY = static_cast<short>(cell.Y + range);
	const auto minX = static_cast<short>(cell.X - range);
	const auto minY = static_cast<short>(cell.Y - range);

	bool green = false;

	for (const auto& baseCell : BuildingTypeExt::BaseNormalCells)
	{
		if (baseCell.X >= minX && baseCell.Y >= minY && baseCell.X <= maxX && baseCell.Y <= maxY)
		{
			green = true;
			break;
		}
	}

	const auto foot = ProximityTemp::Exist;
	ProximityTemp::Exist = false;
	const bool land = pCell->LandType != LandType::Water;
	const auto frames = land ? pRules->PlacementGrid_LandFrames.Get() : pRules->PlacementGrid_WaterFrames.Get();
	auto frame = foot ? frames.X : (green ? frames.Z : frames.Y);

	if (frame >= Make_Global<SHPStruct*>(0x8A03FC)->Frames) // place.shp
		frame = 0;

	R->EDI(frame);
	return 0;
}

// AIConstructionYard Hook #4-1 -> sub_443C60 - Prohibit AI from building construction yard and clean up invalid walls nodes.
DEFINE_HOOK(0x4451F8, BuildingClass_KickOutUnit_CleanUpAIBuildingSpace, 0x6)
{
	enum { BuildSucceeded = 0x4454D4, CanNotBuild = 0x4454E6, BuildFailed = 0x445696 };

	GET(BaseNodeClass* const, pBaseNode, EBX);
	GET(BuildingClass* const, pBuilding, EDI);
	GET(BuildingClass* const, pFactory, ESI);
	GET(const CellStruct, topLeftCell, EDX);

	const auto pBuildingType = pBuilding->Type;
	const auto pTypeExt = BuildingTypeExt::Fetch(pBuildingType);

	if (pTypeExt->LimboBuild)
	{
		BuildingTypeExt::CreateLimboBuilding(pBuilding, pBuildingType, pFactory->Owner, pTypeExt->LimboBuildID);

		if (pBaseNode)
		{
			pBaseNode->Placed = true;
			pBaseNode->Attempts = 0;

			if (pFactory->Owner->ProducingBuildingTypeIndex == pBuildingType->ArrayIndex)
				pFactory->Owner->ProducingBuildingTypeIndex = -1;
		}

		PlayConstructionYardAnim(pFactory);
		return BuildSucceeded;
	}

	// Prohibit AI from building construction yard
	if (RulesExt::Global()->AIForbidConYard && pBuildingType->ConstructionYard)
	{
		if (pBaseNode)
		{
			pBaseNode->Placed = true;
			pBaseNode->Attempts = 0;
		}

		return BuildFailed;
	}

	// Clean up invalid walls nodes
	if (RulesExt::Global()->AICleanWallNode && pBuildingType->Wall)
	{
		auto notValidWallNode = [topLeftCell]()
		{
			const auto pCell = MapClass::Instance.GetCellAt(topLeftCell);

			for (int i = 0; i < 8; ++i)
			{
				if (const auto pAdjBuilding = pCell->GetNeighbourCell(static_cast<FacingType>(i))->GetBuilding())
				{
					if (pAdjBuilding->Type->ProtectWithWall)
						return false;
				}
			}

			return true;
		};

		if (notValidWallNode())
			return CanNotBuild;
	}

	return 0;
}

// AIConstructionYard Hook #4-2 -> sub_42EB50 - Prohibit AI from building construction yard.
DEFINE_HOOK(0x42EB8E, BaseClass_GetBaseNodeIndex_CheckValidBaseNode, 0x6)
{
	enum { Valid = 0x42EBC3, Invalid = 0x42EBAE };

	GET(BaseClass* const, pBase, ESI);
	GET(BaseNodeClass* const, pBaseNode, EAX);

	if (pBaseNode->Placed)
	{
		const auto index = pBaseNode->BuildingTypeIndex;

		if (index >= 0 && index < BuildingTypeClass::Array.Count)
		{
			const auto pType = BuildingTypeClass::Array.Items[index];

			if ((RulesExt::Global()->AIForbidConYard && pType->ConstructionYard)
				|| BuildingTypeExt::Fetch(pType)->LimboBuild)
			{
				return Invalid;
			}
		}
	}

	return reinterpret_cast<bool(__thiscall*)(HouseClass*, BaseNodeClass*)>(0x50CAD0)(pBase->Owner, pBaseNode) ? Valid : Invalid;
}

// AIConstructionYard Hook #5-1 -> sub_588570 - Only expand walls on nodes.
DEFINE_HOOK(0x5885D1, MapClass_BuildingToFirestormWall_SkipExtraWalls, 0x6)
{
	enum { NextDirection = 0x588730 };

	GET_STACK(const HouseClass* const, pHouse, STACK_OFFSET(0x38, 0x8));
	GET_STACK(const int, count, STACK_OFFSET(0x38, -0x24));

	if (pHouse->IsControlledByHuman() || !RulesExt::Global()->AINodeWallsOnly || count)
		return 0;

	GET(const CellStruct, cell, EBX);
	GET(const BuildingTypeClass* const, pType, EBP);

	const auto index = pType->ArrayIndex;
	const auto& nodes = pHouse->Base.BaseNodes;

	for (const auto& pNode : nodes)
	{
		if (pNode.MapCoords == cell && pNode.BuildingTypeIndex == index)
			return 0;
	}

	return NextDirection;
}

// AIConstructionYard Hook #5-2 -> sub_588750 - Only expand walls on nodes.
DEFINE_HOOK(0x5887C1, MapClass_BuildingToWall_SkipExtraWalls, 0x6)
{
	enum { NextDirection = 0x588935 };

	GET_STACK(const HouseClass* const, pHouse, STACK_OFFSET(0x3C, 0x8));
	GET_STACK(const int, count, STACK_OFFSET(0x3C, -0x2C));

	if (pHouse->IsControlledByHuman() || !RulesExt::Global()->AINodeWallsOnly || count)
		return 0;

	GET(const CellStruct, cell, EDX);
	GET(const BuildingTypeClass* const, pType, EDI);

	const auto index = pType->ArrayIndex;
	const auto& nodes = pHouse->Base.BaseNodes;

	for (const auto& pNode : nodes)
	{
		if (pNode.MapCoords == cell && pNode.BuildingTypeIndex == index)
			return 0;
	}

	return NextDirection;
}

DEFINE_HOOK(0x6A8E34, StripClass_Update_LimboBuild, 0x7)
{
	enum { SkipGameCode = 0x6A8F49 };

	GET(BuildingClass* const, pBuilding, ESI);

	return BuildingTypeExt::BuildLimboBuilding(pBuilding) ? SkipGameCode : 0;
}
#pragma region AIAdjacentMax

DEFINE_HOOK_AGAIN(0x42EB6A, BaseClass_GetBaseNodeIndex_AIAdjacentMax, 0x8);
DEFINE_HOOK(0x42EBA2, BaseClass_GetBaseNodeIndex_AIAdjacentMax, 0x8)
{
	GET(BaseClass*, pThis, ESI);
	GET(const int, nodeIdx, EDI);

	bool isValid = reinterpret_cast<bool(__thiscall*)(BaseClass*, int)>(0x42E780)(pThis, nodeIdx);
	const int rangeLimit = SessionClass::IsCampaign()
		? RulesExt::Global()->AIAdjacentMax_Campaign.Get(RulesExt::Global()->AIAdjacentMax.Get())
		: RulesExt::Global()->AIAdjacentMax.Get();

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
