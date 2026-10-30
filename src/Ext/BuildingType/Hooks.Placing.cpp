#include "Body.h"
#include "ProximityTemp.h"

#include <Ext/House/Body.h>
#include <Ext/Rules/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/TerrainType/Body.h>

#include <EventClass.h>

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

// Forward declarations: helpers are defined further below in this file (base layout); Mix defines them before first use.
static inline void ClearPlacingBuildingData(PlacingBuildingStruct* const pPlace);
template <bool slam = false>
static inline void PlayConstructionYardAnim(BuildingClass* const pFactory);
static inline bool CheckBuildingFoundation(BuildingTypeClass* const pBuildingType, const CellStruct topLeftCell, HouseClass* const pHouse, bool& noOccupy);

/*
	In sub_443C60

	- Buildable-upon TechnoTypes Hook #6 -> Try to clean up the building space when AI is building
	- AIConstructionYard Hook #4-1 -> Prohibit AI from building construction yard and clean up invalid walls nodes.
	- Limbo Build Hook #2 -> Skip check limbo building
*/
DEFINE_HOOK(0x4451F8, BuildingClass_KickOutUnit_CleanUpAIBuildingSpace, 0x6)
{
	enum { CanBuild = 0x4452F0, TemporarilyCanNotBuild = 0x445237, CanNotBuild = 0x4454E6, BuildSucceeded = 0x4454D4, BuildFailed = 0x445696 };

	GET(BaseNodeClass* const, pBaseNode, EBX);
	GET(BuildingClass* const, pBuilding, EDI);
	GET(BuildingClass* const, pFactory, ESI);
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

	const auto pHouse = pFactory->Owner;

	if (!RulesExt::Global()->ExtendedBuildingPlacing)
		return 0;

	if (topLeftCell != CellStruct::Empty && !pBuildingType->PlaceAnywhere)
	{
		if (!pBuildingType->PowersUpBuilding[0])
		{
			bool noOccupy = true;
			bool canBuild = CheckBuildingFoundation(pBuildingType, topLeftCell, pHouse, noOccupy);
			const auto pHouseExt = HouseExt::Fetch(pHouse);
			auto& place = pBuildingType->BuildCat != BuildCat::Combat ? pHouseExt->Common : pHouseExt->Combat;

			do
			{
				if (canBuild)
				{
					if (noOccupy)
						break; // Can Build

					do
					{
						if (topLeftCell != place.TopLeft || pBuildingType != place.Type) // New command
						{
							place.Type = pBuildingType;
							place.DrawType = pBuildingType;
							place.TopLeft = topLeftCell;
						}

						if (!place.Timer.HasTimeLeft())
						{
							place.Timer.Start(40);

							if (BuildingTypeExt::CleanUpBuildingSpace(pBuildingType, topLeftCell, pHouse))
								break; // No place for cleaning
						}

						return TemporarilyCanNotBuild;
					}
					while (false);
				}

				ClearPlacingBuildingData(&place);
				return CanNotBuild;
			}
			while (false);

			ClearPlacingBuildingData(&place);
		}
		else
		{
			const auto pCell = MapClass::Instance.GetCellAt(topLeftCell);
			const auto pCellBuilding = pCell->GetBuilding();

			if (!pCellBuilding || !pCellBuilding->CanUpgradeBuilding(pBuildingType, pHouse))
				return CanNotBuild;
		}
	}

	if (pBuilding->Unlimbo(CoordStruct{ (topLeftCell.X << 8) + 128, (topLeftCell.Y << 8) + 128, 0 }, DirType::North))
	{
		PlayConstructionYardAnim(pFactory);
		return CanBuild;
	}

	return CanNotBuild;
}

/*
	In sub_7393C0

	- Buildable-upon TechnoTypes Hook #9-1 -> Try to clean up the building space when is deploying
*/
DEFINE_HOOK(0x73946C, UnitClass_TryToDeploy_CleanUpDeploySpace, 0x6)
{
	enum { CanDeploy = 0x73958A, TemporarilyCanNotDeploy = 0x73953B, CanNotDeploy = 0x7394E0 };

	GET(UnitClass* const, pThis, EBP);
	GET(CellStruct, topLeftCell, ESI);

	if (!RulesExt::Global()->ExtendedBuildingPlacing)
		return 0;

	const auto pTechnoExt = TechnoExt::Fetch(pThis);
	const auto pBuildingType = pThis->Type->DeploysInto;
	const auto pHouseExt = HouseExt::Fetch(pThis->Owner);
	auto& vec = pHouseExt->OwnedDeployingUnits;

	if (pBuildingType->GetFoundationWidth() > 2 || pBuildingType->GetFoundationHeight(false) > 2)
		topLeftCell -= CellStruct { 1, 1 };

	R->Stack<CellStruct>(STACK_OFFSET(0x28, -0x14), topLeftCell);

	if (!pBuildingType->PlaceAnywhere)
	{
		bool noOccupy = true;
		bool canBuild = CheckBuildingFoundation(pBuildingType, topLeftCell, pThis->Owner, noOccupy);

		do
		{
			if (canBuild)
			{
				if (noOccupy)
					break; // Can build

				do
				{
					if (!pTechnoExt->UnitAutoDeployTimer.InProgress())
					{
						if (BuildingTypeExt::CleanUpBuildingSpace(pBuildingType, topLeftCell, pThis->Owner, pThis))
							break; // No place for cleaning

						if (vec.size() == 0 || std::find(vec.begin(), vec.end(), pThis) == vec.end())
							vec.push_back(pThis);

						pTechnoExt->UnitAutoDeployTimer.Start(40);
					}

					if (pThis->PrimaryFacing.Current().GetFacing<256>() != static_cast<size_t>(pBuildingType->DeployFacing))
					{
						const auto pLoco = pThis->Locomotor;

						if (!pLoco->Is_Moving_Now())
							pLoco->Do_Turn(DirStruct(static_cast<DirType>(pBuildingType->DeployFacing)));
					}

					return TemporarilyCanNotDeploy;
				}
				while (false);
			}

			if (vec.size() > 0)
				vec.erase(std::remove(vec.begin(), vec.end(), pThis), vec.end());

			pTechnoExt->UnitAutoDeployTimer.Stop();

			return CanNotDeploy;
		}
		while (false);
	}

	if (vec.size() > 0)
		vec.erase(std::remove(vec.begin(), vec.end(), pThis), vec.end());

	pTechnoExt->UnitAutoDeployTimer.Stop();

	return CanDeploy;
}

/*
	In sub_73FD50

	- Buildable-upon TechnoTypes Hook #9-2 -> Push the owner house into deploy check
*/
DEFINE_HOOK(0x73FF8F, UnitClass_MouseOverObject_ShowDeployCursor, 0x6)
{
	if (RulesExt::Global()->ExtendedBuildingPlacing) // This IF check is not so necessary
	{
		GET(const UnitClass* const, pUnit, ESI);
		LEA_STACK(HouseClass**, pHousePtr, STACK_OFFSET(0x20, -0x20));
		*pHousePtr = pUnit->Owner;
	}

	return 0;
}

/*
	In sub_4C6CB0

	- Buildable-upon TechnoTypes Hook #10 -> Stop deploy when get stop command
*/
DEFINE_HOOK(0x4C7665, EventClass_RespondToEvent_StopDeployInIdleEvent, 0x6)
{
	if (RulesExt::Global()->ExtendedBuildingPlacing) // This IF check is not so necessary
	{
		GET(const UnitClass* const, pUnit, ESI);

		if (pUnit->Type->DeploysInto)
		{
			const auto mission = pUnit->CurrentMission;

			if (mission == Mission::Guard || mission == Mission::Unload)
			{
				if (const auto pHouseExt = HouseExt::TryFetch(pUnit->Owner))
				{
					auto& vec = pHouseExt->OwnedDeployingUnits;

					if (vec.size() > 0)
						vec.erase(std::remove(vec.begin(), vec.end(), pUnit), vec.end());
				}
			}
		}
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

static inline void ClearPlacingBuildingData(PlacingBuildingStruct* const pPlace)
{
	pPlace->Type = nullptr;
	pPlace->DrawType = nullptr;
	pPlace->Times = 0;
	pPlace->Timer.Stop();
	pPlace->TopLeft = CellStruct::Empty;
	pPlace->PlaceType = 0;
}

static inline void ClearCurrentBuildingData(DisplayClass* const pDisplay)
{
	pDisplay->SetActiveFoundation(nullptr);
	pDisplay->CurrentBuilding = nullptr;
	pDisplay->CurrentBuildingType = nullptr;
	pDisplay->CurrentBuildingOwnerArrayIndexCopy = -1;

	if (!Unsorted::ArmageddonMode)
	{
		pDisplay->SetBusyFoundation(nullptr);
		pDisplay->CurrentBuildingCopy = nullptr;
		pDisplay->CurrentBuildingTypeCopy = nullptr;
	}
}

template <bool slam>
static inline void PlayConstructionYardAnim(BuildingClass* const pFactory)
{
	const auto pFactoryType = pFactory->Type;

	if (pFactoryType->ConstructionYard)
	{
		if constexpr (slam)
			VocClass::PlayGlobal(RulesClass::Instance->BuildingSlam, 0x2000, 1.0);

		pFactory->DestroyNthAnim(BuildingAnimSlot::PreProduction);
		pFactory->DestroyNthAnim(BuildingAnimSlot::Idle);

		const bool damaged = pFactory->GetHealthPercentage() <= RulesClass::Instance->ConditionYellow;
		const auto pAnimName = damaged ? pFactoryType->BuildingAnim[8].Damaged : pFactoryType->BuildingAnim[8].Anim;

		if (pAnimName && *pAnimName)
			pFactory->PlayAnim(pAnimName, BuildingAnimSlot::Production, damaged, false);
	}
}

static inline bool CheckBuildingFoundation(BuildingTypeClass* const pBuildingType, const CellStruct topLeftCell, HouseClass* const pHouse, bool& noOccupy)
{
	for (auto pFoundation = pBuildingType->GetFoundationData(true); *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
	{
		if (const auto pCell = MapClass::Instance.TryGetCellAt(topLeftCell + *pFoundation))
		{
			if (!pCell->CanThisExistHere(pBuildingType->SpeedType, pBuildingType, pHouse))
				return false;
		}
	}

	return true;
}

// Place Another Type Helper
namespace PlaceTypeTemp
{
	size_t PlaceType = 0;
}

/*
	In sub_4FB0E0

	- Place Another Type Hook #1 -> Replace the factory product
	- Buildable-upon TechnoTypes Hook #3 -> Hang up place event if there is only infantries and units on the cell
	- Limbo Build Hook #1 -> Skip check limbo building
*/
DEFINE_HOOK(0x4FB1EA, HouseClass_UnitFromFactory_HangUpPlaceEvent, 0x5)
{
	enum { CanBuild = 0x4FB23C, TemporarilyCanNotBuild = 0x4FB5BA, CanNotBuild = 0x4FB35F, BuildSucceeded = 0x4FB649 };

	GET(HouseClass* const, pHouse, EBP);
	GET(TechnoClass* const, pTechno, ESI);
	GET(FactoryClass* const, pPrimary, EBX);
	GET(BuildingClass* const, pFactory, EDI);
	GET_STACK(const CellStruct, topLeftCell, STACK_OFFSET(0x3C, 0x10));

	if (pTechno->WhatAmI() != AbstractType::Building)
	{
		pFactory->SendCommand(RadioCommand::RequestLink, pTechno);

		if (pTechno->Unlimbo(CoordStruct{ (topLeftCell.X << 8) + 128, (topLeftCell.Y << 8) + 128, 0 }, DirType::North))
			return CanBuild;

		return CanNotBuild;
	}

	const auto pDisplay = &DisplayClass::Instance;
	auto pBuilding = static_cast<BuildingClass*>(pTechno);
	auto pBuildingType = pBuilding->Type;
	const auto pBufferBuilding = pBuilding;
	const auto pBufferType = pBuildingType;

	const bool upgrade = pBuildingType->PowersUpBuilding[0];
	const bool expand = RulesExt::Global()->ExtendedBuildingPlacing && !pBuildingType->PlaceAnywhere && !upgrade;
	const size_t placeType = PlaceTypeTemp::PlaceType;

	bool revert = upgrade;

	if (expand)
	{
		bool noOccupy = true;
		bool canBuild = CheckBuildingFoundation(pBuildingType, topLeftCell, pHouse, noOccupy);
		const auto pHouseExt = HouseExt::Fetch(pHouse);
		auto& place = pBufferType->BuildCat != BuildCat::Combat ? pHouseExt->Common : pHouseExt->Combat;

		do
		{
			if (canBuild)
			{
				if (noOccupy)
					break; // Can Build

				do
				{
					if (topLeftCell != place.TopLeft || pBufferType != place.Type) // New command
					{
						place.Type = pBufferType;
						place.DrawType = pBuildingType;
						place.Times = 30;
						place.TopLeft = topLeftCell;
						place.PlaceType = placeType;
					}
					else if (place.Times <= 0)
					{
						break; // Time out
					}

					if (!(place.Times % 5) && BuildingTypeExt::CleanUpBuildingSpace(pBuildingType, topLeftCell, pHouse))
						break; // No place for cleaning

					if (pHouse == HouseClass::CurrentPlayer && place.Times == 30)
						ClearCurrentBuildingData(pDisplay);

					--place.Times;
					place.Timer.Start(8);

					return TemporarilyCanNotBuild;
				}
				while (false);
			}

			revert = place.Times == 30 || !place.Type;
			ClearPlacingBuildingData(&place);

			return CanNotBuild;
		}
		while (false);

		revert = !place.Type;
		ClearPlacingBuildingData(&place);
	}

	if (pBufferType != pBuildingType)
		pBuilding = static_cast<BuildingClass*>(pBuildingType->CreateObject(pHouse));

	pFactory->SendCommand(RadioCommand::RequestLink, pBuilding);

	if (pBuilding->Unlimbo(CoordStruct{ (topLeftCell.X << 8) + 128, (topLeftCell.Y << 8) + 128, 0 }, DirType::North))
	{
		if (pBufferBuilding != pBuilding)
		{
			if (HouseClass::CurrentPlayer == pHouse)
			{
				if (!pDisplay->CurrentBuilding)
				{
				}
				else if (pDisplay->CurrentBuilding == pBufferBuilding)
				{
					pDisplay->CurrentBuilding = pBuilding;
				}

				if (pDisplay->CurrentBuildingType == pBufferType)
					pDisplay->CurrentBuildingType = pBuildingType;

				if (pDisplay->CurrentBuildingCopy == pBufferBuilding)
					pDisplay->CurrentBuildingCopy = pBuilding;

				if (pDisplay->CurrentBuildingTypeCopy == pBufferType)
					pDisplay->CurrentBuildingTypeCopy = pBuildingType;

				if (Make_Global<BuildingClass*>(0xB0FE5C) == pBufferBuilding) // Q
					Make_Global<BuildingClass*>(0xB0FE5C) = pBuilding;

				if (Make_Global<BuildingClass*>(0xB0FE60) == pBufferBuilding) // W
					Make_Global<BuildingClass*>(0xB0FE60) = pBuilding;
			}

			pBufferBuilding->UnInit();
			pPrimary->Object = pBuilding;
			R->ESI(pBuilding);
		}
		return CanBuild;
	}

	if (pBufferBuilding != pBuilding)
		pBuilding->UnInit();

	return CanNotBuild;
}

/*
	In sub_4A91B0

	- Place Another Type Hook #2 -> Replace current building type for check
*/
DEFINE_HOOK(0x4A937D, DisplayClass_CallBuildingPlaceCheck_ReplaceBuildingType, 0x8)
{
	enum { SkipGameCode = 0x4A943A };

	GET(const CellStruct, cell, EBP);

	const auto pDisplay = &DisplayClass::Instance;

	auto updateCurrentFoundation = [pDisplay, cell]()
	{
		if (pDisplay->CurrentFoundation_CenterCell != cell)
		{
			auto oldCell = pDisplay->CurrentFoundation_CenterCell;

			if (oldCell != CellStruct::Empty)
			{
				oldCell += pDisplay->CurrentFoundation_TopLeftOffset;
				pDisplay->MarkFoundation(&oldCell, false);
			}

			auto newCell = cell;

			if (newCell != CellStruct::Empty)
			{
				newCell += pDisplay->CurrentFoundation_TopLeftOffset;
				pDisplay->MarkFoundation(&newCell, true);
			}

			pDisplay->CurrentFoundation_CenterCell = cell;
		}
	};

	updateCurrentFoundation();

	return SkipGameCode;
}

/*
	In sub_4AB9B0

	- Place Another Type Hook #3 -> Keep current foundation center cell
*/
DEFINE_HOOK(0x4AB9FF, DisplayClass_LeftMouseButtonUp_MaintainCell, 0x6)
{
	enum { ContinueCheckUpgrade = 0x4ABA84, SkipCheckUpgrade = 0x4ABAA4 };

	const auto pCellStruct = &DisplayClass::Instance.CurrentFoundation_CenterCell;

	R->EBX(pCellStruct);

	const auto pType = DisplayClass::Instance.CurrentBuildingType;

	if (pType->WhatAmI() != AbstractType::BuildingType || !static_cast<BuildingTypeClass*>(pType)->PowersUpBuilding[0])
		return SkipCheckUpgrade;

	const auto pCellBuilding = MapClass::Instance.GetCellAt(*pCellStruct)->GetBuilding();

	if (!pCellBuilding)
		return SkipCheckUpgrade;
	else
		R->ESI(pCellBuilding);

	return ContinueCheckUpgrade;
}

/*
	In sub_4AB9B0

	- Place Another Type Hook #4 -> Replace event
*/
DEFINE_HOOK(0x4ABAC0, DisplayClass_LeftMouseButtonUp_ReplaceBuildingType, 0x6)
{
	enum { SkipGameCode = 0x4ABBB3 };

	const auto pDisplay = &DisplayClass::Instance;

	const auto centerCell = pDisplay->CurrentFoundation_CenterCell;
	const auto placeCell = centerCell + pDisplay->CurrentFoundation_TopLeftOffset;

	const auto pPlace = pDisplay->CurrentBuilding;
	const auto pType = pPlace->GetType(); // Should not use CurrentBuildingType
	int placeType = 0;
	{
		const auto pTechnoType = TechnoTypeExt::GetTechnoType(pType);
		placeType |= (pTechnoType && pTechnoType->Naval);
	}

	const int arrayIndex = pType->GetArrayIndex();
	const auto absType = pPlace->WhatAmI();

	EventClass::OutList.Add(EventClass(
		HouseClass::CurrentPlayer->ArrayIndex,
		EventType::Place,
		absType,
		arrayIndex,
		placeType,
		placeCell
	));

	return SkipGameCode;
}

/*
	In sub_4C6CB0

	- Place Another Type Hook #5 -> Replace placing action
*/
DEFINE_HOOK(0x4C70E1, EventClass_RespondToEvent_SetPlaceType, 0x8)
{
	enum { SkipGameCode = 0x4C7110 };

	GET(EventClass* const, pThis, ESI);
	GET(HouseClass* const, pHouse, EDI);

	const auto cell = pThis->Place.Location;
	const int flags = pThis->Place.IsNaval;
	const bool isNaval = flags & 1;
	const int arrayIndex = pThis->Place.HeapID;
	const auto absType = pThis->Place.RTTIType;

	if (absType == AbstractType::Building || absType == AbstractType::BuildingType)
		PlaceTypeTemp::PlaceType = static_cast<size_t>(flags);

	pHouse->UnitFromFactory(absType, arrayIndex, isNaval, &cell);
	PlaceTypeTemp::PlaceType = 0;

	return SkipGameCode;
}

/*
	In sub_4FB0E0

	- Buildable-upon TechnoTypes Hook #4-1 -> Check whether need to skip the replace command
*/
DEFINE_HOOK(0x4FB395, HouseClass_UnitFromFactory_SkipMouseReturn, 0x6)
{
	enum { SkipGameCode = 0x4FB489, CheckMouseCoords = 0x4FB3E3 };

	if (!RulesExt::Global()->ExtendedBuildingPlacing)
		return 0;

	R->EBX(0);

	if (!DisplayClass::Instance.CurrentBuildingTypeCopy)
		return SkipGameCode;

	R->ECX(DisplayClass::Instance.CurrentBuildingTypeCopy);
	return CheckMouseCoords;
}

/*
	In sub_4FB0E0

	- Buildable-upon TechnoTypes Hook #4-2 -> Check whether need to skip the clear command
*/
DEFINE_HOOK(0x4FB339, HouseClass_UnitFromFactory_SkipMouseClear, 0x6)
{
	enum { SkipGameCode = 0x4FB4A0 };

	GET(TechnoClass* const, pTechno, ESI);

	if (RulesExt::Global()->ExtendedBuildingPlacing)
	{
		if (const auto pBuilding = abstract_cast<BuildingClass*, true>(pTechno))
		{
			if (const auto pCurrentType = abstract_cast<BuildingTypeClass*>(DisplayClass::Instance.CurrentBuildingType))
			{
				if (!BuildingTypeExt::IsSameBuildingType(pBuilding->Type, pCurrentType))
					return SkipGameCode;
			}
		}
	}

	return 0;
}

/*
	In sub_4FAA10

	- Buildable-upon TechnoTypes Hook #4-3 -> Check whether need to skip the clear command
*/
DEFINE_HOOK(0x4FAB83, HouseClass_AbandonProductionOf_SkipMouseClear, 0x7)
{
	enum { SkipGameCode = 0x4FABA4 };

	GET(const int, index, EBX);

	if (RulesExt::Global()->ExtendedBuildingPlacing && index >= 0)
	{
		if (const auto pCurrentBuildingType = abstract_cast<BuildingTypeClass*>(DisplayClass::Instance.CurrentBuildingType))
		{
			if (!BuildingTypeExt::IsSameBuildingType(BuildingTypeClass::Array.Items[index], pCurrentBuildingType))
				return SkipGameCode;
		}
	}

	return 0;
}

/*
	In sub_4C9FF0

	- Buildable-upon TechnoTypes Hook #5 -> Restart timer and clear buffer when abandon building production
*/
DEFINE_HOOK(0x4CA05B, FactoryClass_AbandonProduction_AbandonCurrentBuilding, 0x5)
{
	GET(FactoryClass*, pFactory, ESI);

	if (RulesExt::Global()->ExtendedBuildingPlacing)
	{
		const auto pBuilding = abstract_cast<BuildingClass*>(pFactory->Object);

		if (!pBuilding)
			return 0;

		const auto pHouseExt = HouseExt::Fetch(pFactory->Owner);
		auto& place = pBuilding->Type->BuildCat != BuildCat::Combat ? pHouseExt->Common : pHouseExt->Combat;
		ClearPlacingBuildingData(&place);
	}

	return 0;
}

/*
	In sub_4F8440

	- Buildable-upon TechnoTypes Hook #11 -> Check whether can place again in each house
	- Auto Build Hook #1 -> Automatic place buildings
*/
DEFINE_HOOK(0x4F8DB1, HouseClass_Update_CheckHangUpBuilding, 0x6)
{
	GET(HouseClass* const, pHouse, ESI);

	if (!pHouse->IsControlledByHuman())
		return 0;

	// Automatic place buildings
	if (pHouse == HouseClass::CurrentPlayer && (pHouse->RecheckTechTree || !(Unsorted::CurrentFrame % 15)))
	{
		if (const auto pFactory = pHouse->Primary_ForBuildings)
		{
			if (pFactory->IsDone() && !HouseExt::Fetch(pHouse)->Common.Type)
			{
				if (const auto pBuilding = abstract_cast<BuildingClass*>(pFactory->Object))
					BuildingTypeExt::AutoPlaceBuilding(pBuilding);
			}
		}

		if (const auto pFactory = pHouse->Primary_ForDefenses)
		{
			if (pFactory->IsDone() && !HouseExt::Fetch(pHouse)->Combat.Type)
			{
				if (const auto pBuilding = abstract_cast<BuildingClass*>(pFactory->Object))
					BuildingTypeExt::AutoPlaceBuilding(pBuilding);
			}
		}
	}

	if (!RulesExt::Global()->ExtendedBuildingPlacing)
		return 0;

	const auto pHouseExt = HouseExt::Fetch(pHouse);
	auto buildCurrent = [&pHouse, &pHouseExt](BuildingTypeClass* pType, CellStruct cell, size_t placeType)
	{
		if (!pType)
			return;

		auto currentCanBuild = [&pHouse, &pType]() -> const bool
		{
			auto const bitsOwners = pType->GetOwners();

			for(auto const& pConYard : pHouse->ConYards)
			{
				if (pConYard->InLimbo || !pConYard->HasPower || pConYard->Deactivated)
					continue;

				if (pConYard->CurrentMission == Mission::Selling || pConYard->QueuedMission == Mission::Selling)
					continue;

				const auto pConYardType = pConYard->Type;

				if (pConYardType->Factory != AbstractType::BuildingType || !pConYardType->InOwners(bitsOwners))
					continue;

				return true;
			}

			return false;
		};

		if (!currentCanBuild())
		{
			ClearPlacingBuildingData(pType->BuildCat != BuildCat::Combat ? &pHouseExt->Common : &pHouseExt->Combat);

			if (pHouse == HouseClass::CurrentPlayer)
				VoxClass::Play(GameStrings::EVA_CannotDeployHere);
		}
		else if (pHouse == HouseClass::CurrentPlayer) // Prevent unexpected wrong event
		{
			const int place = static_cast<int>(placeType);
			const auto arrayIndex = pType->GetArrayIndex();
			EventClass::OutList.Add(EventClass(pHouse->ArrayIndex, EventType::Place, AbstractType::Building, arrayIndex, place, cell));
		}
	};

	auto& commonPlace = pHouseExt->Common;

	if (commonPlace.Timer.Completed())
	{
		commonPlace.Timer.Stop();
		buildCurrent(commonPlace.Type, commonPlace.TopLeft, commonPlace.PlaceType);
	}

	auto& combatPlace = pHouseExt->Combat;

	if (combatPlace.Timer.Completed())
	{
		combatPlace.Timer.Stop();
		buildCurrent(combatPlace.Type, combatPlace.TopLeft, combatPlace.PlaceType);
	}

	if (pHouseExt->OwnedDeployingUnits.size() > 0)
	{
		auto& vec = pHouseExt->OwnedDeployingUnits;

		for (auto it = vec.begin(); it != vec.end(); )
		{
			const auto pUnit = *it;

			if (!pUnit->InLimbo
				&& pUnit->IsOnMap
				&& !pUnit->IsSinking
				&& pUnit->Owner == pHouse
				&& !pUnit->Destination
				&& !pUnit->ParasiteEatingMe
				&& !pUnit->TemporalTargetingMe
				&& pUnit->Type->DeploysInto)
			{
				const auto mission = pUnit->CurrentMission;

				if (mission == Mission::Unload)
				{
					++it;
					continue;
				}
				else if (mission == Mission::Guard)
				{
					if (const auto pExt = TechnoExt::Fetch(pUnit))
					{
						if (!(pExt->UnitAutoDeployTimer.GetTimeLeft() % 8))
							pUnit->QueueMission(Mission::Unload, true);

						++it;
						continue;
					}
				}
			}

			it = vec.erase(it);
		}
	}

	return 0;
}

/*
	In sub_6D5030

	- Buildable-upon TechnoTypes Hook #12 -> Draw the placing building preview
*/
DEFINE_HOOK(0x6D504C, TacticalClass_DrawPlacement_DrawPlacingPreview, 0x6)
{
	if (!RulesExt::Global()->ExtendedBuildingPlacing)
		return 0;

	const auto pPlayer = HouseClass::CurrentPlayer;
	const auto pDisplay = &DisplayClass::Instance;

	auto drawImage = [&pDisplay](BuildingTypeClass* const pType, HouseClass* const pHouse, const CellStruct cell)
	{
		const auto pCell = pDisplay->TryGetCellAt(cell);

		if (!pCell || cell == CellStruct::Empty || pType->PowersUpBuilding[0])
			return;

		auto pImage = pType->LoadBuildup();
		int imageFrame = 0;

		if (pImage)
			imageFrame = ((pImage->Frames / 2) - 1);
		else
			pImage = pType->GetImage();

		if (!pImage)
			return;

		BlitterFlags blitFlags = BlitterFlags::TransLucent75 | BlitterFlags::Centered | BlitterFlags::Nonzero | BlitterFlags::MultiPass;
		auto rect = DSurface::Temp->GetRect();
		rect.Height -= 32;
		auto point = TacticalClass::Instance->CoordsToClient(CellClass::Cell2Coord(cell, (1 + pCell->GetFloorHeight(Point2D::Empty)))).first;
		point.Y -= 15;

		const auto ColorSchemeIndex = pHouse->ColorSchemeIndex;
		const auto Palettes = pType->Palette;
		const auto pColor = Palettes ? Palettes->Items[ColorSchemeIndex] : ColorScheme::Array.Items[ColorSchemeIndex];
		DSurface::Temp->DrawSHP(pColor->LightConvert, pImage, imageFrame, &point, &rect, blitFlags, 0, 0, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
	};

	for (const auto& pHouse : HouseClass::Array)
	{
		if (pPlayer->IsObserver() || pHouse->IsAlliedWith(pPlayer))
		{
			const auto pHouseExt = HouseExt::Fetch(pHouse);

			if (const auto pType = abstract_cast<BuildingTypeClass*>(pDisplay->CurrentBuildingTypeCopy))
				drawImage(pType, pHouse, (pDisplay->CurrentFoundationCopy_TopLeftOffset + pDisplay->CurrentFoundationCopy_CenterCell));

			if (const auto pType = pHouseExt->Common.DrawType)
				drawImage(pType, pHouse, pHouseExt->Common.TopLeft);

			if (const auto pType = pHouseExt->Combat.DrawType)
				drawImage(pType, pHouse, pHouseExt->Combat.TopLeft);

			if (pHouseExt->OwnedDeployingUnits.size() <= 0)
				continue;

			for (const auto& pUnit : pHouseExt->OwnedDeployingUnits)
			{
				const auto pType = pUnit->Type->DeploysInto;

				if (!pType)
					continue;

				auto displayCell = CellClass::Coord2Cell(pUnit->GetCoords()); // pUnit->GetMapCoords();

				if (pType->GetFoundationWidth() > 2 || pType->GetFoundationHeight(false) > 2)
					displayCell -= CellStruct { 1, 1 };

				drawImage(pType, pHouse, displayCell);
			}
		}
	}

	return 0;
}

/*
	In sub_6A8B30

	- Limbo Build Hook #3 -> Automatic spawn buildings
	- Auto Build Hook #2 -> Automatic place buildings
*/
DEFINE_HOOK(0x6A8E34, StripClass_Update_AutoBuildBuildings, 0x7)
{
	GET(BuildingClass* const, pBuilding, ESI);

	BuildingTypeExt::AutoPlaceBuilding(pBuilding);

	return 0;
}
