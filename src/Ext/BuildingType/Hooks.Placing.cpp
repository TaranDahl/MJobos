#include "Body.h"

#include <Ext/Rules/Body.h>
#include <Ext/TerrainType/Body.h>
#include <Ext/TechnoType/Body.h>

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

// Buildable-upon TerrainTypes/TechnoTypes Hook #3 - Remove them when buildings are placed on them.
DEFINE_HOOK(0x5684B1, MapClass_PlaceDown_BuildableUponTypes, 0x6)
{
	GET(ObjectClass*, pPlaceObject, EDI);
	GET(CellClass*, pCell, EAX);

	if (pPlaceObject->WhatAmI() == AbstractType::Building)
	{
		for (auto pObject = pCell->FirstObject; pObject; pObject = pObject->NextObject)
		{
			if (const auto pTechno = abstract_cast<TechnoClass*, true>(pObject))
			{
				if (TechnoTypeExt::Fetch(pTechno->GetTechnoType())->CanBeBuiltOn)
				{
					pTechno->KillPassengers(nullptr);
					pTechno->Stun();
					pTechno->Limbo();
					pTechno->UnInit();
				}
			}
			else if (const auto pTerrain = abstract_cast<TerrainClass*, true>(pObject))
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
		return CheckCompleted;

	return 0;
}

/*
	In sub_4A8EB0

	- BaseNormal extra checking Hook #1-5 -> Restore the correct result
*/
DEFINE_HOOK(0x4A904E, MapClass_PassesProximityCheck_RestoreResult, 0x5)
{
	return 0;
}

// Let the game do the proximity and shroud check when the cell which mouse is pointing at has not changed
DEFINE_JUMP(LJMP, 0x4AACD9, 0x4AACF5);
DEFINE_JUMP(LJMP, 0x4A9361, 0x4A9371);

// AIConstructionYard Hook #4-1 -> sub_443C60 - Prohibit AI from building construction yard and clean up invalid walls nodes.
DEFINE_HOOK(0x4451F8, BuildingClass_KickOutUnit_CleanUpAIBuildingSpace, 0x6)
{
	enum { CanNotBuild = 0x4454E6, BuildFailed = 0x445696 };

	GET(BaseNodeClass* const, pBaseNode, EBX);
	GET(BuildingClass* const, pBuilding, EDI);
	GET(const CellStruct, topLeftCell, EDX);

	const auto pBuildingType = pBuilding->Type;

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

	if (RulesExt::Global()->AIForbidConYard && pBaseNode->Placed)
	{
		const auto index = pBaseNode->BuildingTypeIndex;

		if (index >= 0 && index < BuildingTypeClass::Array.Count && BuildingTypeClass::Array.Items[index]->ConstructionYard)
			return Invalid;
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
