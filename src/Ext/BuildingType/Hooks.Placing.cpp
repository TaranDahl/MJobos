#include "Body.h"

#include <Ext/Unit/Body.h>
#include <Ext/House/Body.h>
#include <Ext/Rules/Body.h>
#include <Ext/Scenario/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/TerrainType/Body.h>

#include <EventClass.h>
#include <IsometricTileTypeClass.h>

/*
	In sub_740810

	- AIConstructionYard Hook #1 -> Check number of construction yard before deploy
*/
DEFINE_HOOK(0x740A11, UnitClass_Mission_Guard_AIAutoDeployMCV, 0x6)
{
	enum { SkipGameCode = 0x740A50 };

	GET(UnitClass*, pMCV, ESI);

	return (!RulesExt::Global()->AIAutoDeployMCV && pMCV->Owner->NumConYards > 0) ? SkipGameCode : 0;
}

/*
	In sub_7393C0

	- AIConstructionYard Hook #2 -> Skip useless base center setting
*/
DEFINE_HOOK(0x739889, UnitClass_TryToDeploy_AISetBaseCenter, 0x6)
{
	enum { SkipGameCode = 0x73992B };

	GET(UnitClass*, pMCV, EBP);

	return (!RulesExt::Global()->AISetBaseCenter && pMCV->Owner->NumConYards > 1) ? SkipGameCode : 0;
}

/*
	In sub_4FD500

	- AIConstructionYard Hook #3 -> Update better base center
*/
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

/*
	In sub_588570

	- AIConstructionYard Hook #5-1 -> Only expand walls on nodes
*/
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

/*
	In sub_588750

	- AIConstructionYard Hook #5-2 -> Only expand walls on nodes
*/
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

/*
	In sub_6D5730

	- Buildable-upon TerrainTypes Hook #2 -> Draw laser fence placement even if they are on the way
*/
DEFINE_HOOK(0x6D57C1, TacticalClass_DrawLaserFencePlacement_BuildableTerrain, 0x9)
{
	enum { ContinueChecks = 0x6D57D2, DontDraw = 0x6D59A6 };

	GET(CellClass*, pCell, ESI);

	if (auto const pTerrain = pCell->GetTerrain(false))
	{
		auto const pType = pTerrain->Type;
		auto const pTypeExt = TerrainTypeExt::Fetch(pType);
		bool const canBuild = pType->SpawnsTiberium
			? pTypeExt->CanBeBuiltOn.Get(RulesExt::Global()->Tibtree_CanBeBuiltOn)
			: pTypeExt->CanBeBuiltOn.Get(RulesExt::Global()->Terrain_CanBeBuiltOn);

		return canBuild ? ContinueChecks : DontDraw;
	}

	return ContinueChecks;
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

static inline bool IsSameFenceType(const BuildingTypeClass* const pPostType, const BuildingTypeClass* const pFenceType)
{
	if (const auto pSpecificType = BuildingTypeExt::Fetch(pPostType)->LaserFencePost_Fence.Get())
	{
		if (pSpecificType != pFenceType)
			return false;
	}
	else
	{
		const auto count = BuildingTypeClass::Array.Count;

		for (int i = 0; i < count; ++i)
		{
			const auto pSearchType = BuildingTypeClass::Array.Items[i];

			if (pSearchType->LaserFence)
			{
				if (pSearchType != pFenceType)
					return false;

				break;
			}
		}
	}

	return true;
}

static inline bool CheckCanNotExistHere(FootClass* const pTechno, HouseClass* const pOwner,
	bool expand, bool& skipFlag, bool& builtOnCanBeBuiltOn, bool& landFootOnly, bool canBuildUnderUnits)
{
	if (pTechno == UnitExt::Deployer)
	{
		skipFlag = true;
		return false;
	}

	const auto pTechnoType = pTechno->GetTechnoType();

	if (canBuildUnderUnits || TechnoTypeExt::Fetch(pTechnoType)->CanBeBuiltOn)
		builtOnCanBeBuiltOn = true;
	else if (!expand || pTechnoType->Speed <= 0 || !BuildingTypeExt::CheckOccupierCanLeave(pOwner, pTechno->Owner))
		return true;
	else
		landFootOnly = true;

	return false;
}

/*
	In sub_47C620

	- Buildable-upon TerrainTypes Hook #1 -> Allow placing buildings on top of them
	- Buildable-upon TechnoTypes Hook #1 -> Rewrite and check whether allow placing buildings on top of them
	- Customized Laser Fence Hook #1 -> Forbid placing laser fence post on inappropriate laser fence
	- Fix DeploysInto Desync core Hook -> Exclude the specific unit who want to deploy
*/
DEFINE_HOOK(0x47C640, CellClass_CanThisExistHere_IgnoreSomething, 0x6)
{
	enum { CanNotExistHere = 0x47C6D1, CanExistHere = 0x47C6A0 };

	GET(const CellClass* const, pCell, EDI);
	GET(const BuildingTypeClass* const, pBuildingType, EAX);
	GET_STACK(HouseClass* const, pOwner, STACK_OFFSET(0x18, 0xC));

	ProximityTemp::Exist = false;

	if (!Game::IsActive)
		return CanExistHere;

	const bool expand = RulesExt::Global()->ExtendedBuildingPlacing.Get();
	const bool canBuildUnderUnits = BuildingTypeExt::Fetch(pBuildingType)->CanBuildUnderUnits.Get();
	bool landFootOnly = false;
	auto isTerrainBuildable = [](TerrainClass* pTerrain) -> bool
	{
		auto const pType = pTerrain->Type;
		auto const pTypeExt = TerrainTypeExt::Fetch(pType);
		return pType->SpawnsTiberium
			? pTypeExt->CanBeBuiltOn.Get(RulesExt::Global()->Tibtree_CanBeBuiltOn)
			: pTypeExt->CanBeBuiltOn.Get(RulesExt::Global()->Terrain_CanBeBuiltOn);
	};

	if (pBuildingType->LaserFence)
	{
		for (auto pObject = pCell->FirstObject; pObject; pObject = pObject->NextObject)
		{
			switch (pObject->WhatAmI())
			{
				case AbstractType::Building:
				{
					if (!TechnoTypeExt::Fetch(static_cast<BuildingClass*>(pObject)->Type)->CanBeBuiltOn)
						return CanNotExistHere;

					break;
				}

				case AbstractType::Terrain:
				{
					if (!isTerrainBuildable(static_cast<TerrainClass*>(pObject)))
						return CanNotExistHere;

					break;
				}

				default:
				{
					break;
				}
			}
		}
	}
	else if (pBuildingType->LaserFencePost || pBuildingType->Gate)
	{
		bool skipFlag = UnitExt::Deployer ? UnitExt::Deployer->CurrentMapCoords == pCell->MapCoords : false;
		bool builtOnCanBeBuiltOn = false;

		for (auto pObject = pCell->FirstObject; pObject; pObject = pObject->NextObject)
		{
			switch (pObject->WhatAmI())
			{
				case AbstractType::Aircraft:
				{
					if (!canBuildUnderUnits && !TechnoTypeExt::Fetch(static_cast<AircraftClass*>(pObject)->Type)->CanBeBuiltOn)
						return CanNotExistHere;

					builtOnCanBeBuiltOn = true;
					break;
				}

				case AbstractType::Building:
				{
					const auto pBuilding = static_cast<BuildingClass*>(pObject);
					const auto pType = pBuilding->Type;

					if (TechnoTypeExt::Fetch(pType)->CanBeBuiltOn)
						builtOnCanBeBuiltOn = true;
					else if (pOwner != pBuilding->Owner || !pType->LaserFence)
						return CanNotExistHere;
					else if (pBuildingType->LaserFencePost && !IsSameFenceType(pBuildingType, pType))
						return CanNotExistHere;

					break;
				}

				case AbstractType::Infantry:
				case AbstractType::Unit:
				{
					if (CheckCanNotExistHere(static_cast<FootClass*>(pObject), pOwner, expand, skipFlag, builtOnCanBeBuiltOn, landFootOnly, canBuildUnderUnits))
						return CanNotExistHere;

					break;
				}

				case AbstractType::Terrain:
				{
					if (!isTerrainBuildable(static_cast<TerrainClass*>(pObject)))
						return CanNotExistHere;

					builtOnCanBeBuiltOn = true;
					break;
				}

				default:
				{
					break;
				}
			}
		}

		if (!landFootOnly && !builtOnCanBeBuiltOn && (pCell->OccupationFlags & (skipFlag ? 0x1F : 0x3F)))
		{
			if (expand)
				landFootOnly = true;
			else
				return CanNotExistHere;
		}
	}
	else if (pBuildingType->ToTile)
	{
		const auto isoTileTypeIndex = pCell->IsoTileTypeIndex;

		if (isoTileTypeIndex >= 0 && isoTileTypeIndex < IsometricTileTypeClass::Array.Count && !IsometricTileTypeClass::Array.Items[isoTileTypeIndex]->Morphable)
			return CanNotExistHere;

		for (auto pObject = pCell->FirstObject; pObject; pObject = pObject->NextObject)
		{
			if (const auto pBuilding = abstract_cast<BuildingClass*, true>(pObject))
			{
				if (!TechnoTypeExt::Fetch(pBuilding->Type)->CanBeBuiltOn)
					return CanNotExistHere;
			}
		}
	}
	else
	{
		bool skipFlag = UnitExt::Deployer ? UnitExt::Deployer->CurrentMapCoords == pCell->MapCoords : false;
		bool builtOnCanBeBuiltOn = false;

		for (auto pObject = pCell->FirstObject; pObject; pObject = pObject->NextObject)
		{
			switch (pObject->WhatAmI())
			{
				case AbstractType::Aircraft:
				{
					if (canBuildUnderUnits)
					{
						builtOnCanBeBuiltOn = true;
						break;
					}

					// No break
				}
				case AbstractType::Building:
				{
					if (!TechnoTypeExt::Fetch(pObject->GetTechnoType())->CanBeBuiltOn)
						return CanNotExistHere;

					builtOnCanBeBuiltOn = true;
					break;
				}

				case AbstractType::Infantry:
				case AbstractType::Unit:
				{
					if (CheckCanNotExistHere(static_cast<FootClass*>(pObject), pOwner, expand, skipFlag, builtOnCanBeBuiltOn, landFootOnly, canBuildUnderUnits))
						return CanNotExistHere;

					break;
				}

				case AbstractType::Terrain:
				{
					if (!isTerrainBuildable(static_cast<TerrainClass*>(pObject)))
						return CanNotExistHere;

					builtOnCanBeBuiltOn = true;
					break;
				}

				default:
				{
					break;
				}
			}
		}

		if (!landFootOnly && !builtOnCanBeBuiltOn && (pCell->OccupationFlags & (skipFlag ? 0x1F : 0x3F)))
		{
			if (expand)
				landFootOnly = true;
			else
				return CanNotExistHere;
		}
	}

	if (landFootOnly)
		ProximityTemp::Exist = true;

	return CanExistHere; // Continue check the overlays .etc
}

/*
	In sub_47EC90

	- Buildable-upon TechnoTypes Hook #2-1 -> Record cell before draw it then skip vanilla AltFlags check
*/
DEFINE_HOOK(0x47EEBC, CellClass_DrawPlaceGrid_RecordCell, 0x6)
{
	enum { DontDrawAlt = 0x47EF1A, DrawVanillaAlt = 0x47EED6 };

	GET(CellClass* const, pCell, ESI);
	GET(const bool, zero, EDX);

	const BlitterFlags flags = BlitterFlags::Centered | BlitterFlags::bf_400;
	ProximityTemp::CurrentCell = pCell;

	if (!(pCell->AltFlags & AltCellFlags::ContainsBuilding))
	{
		if (!RulesExt::Global()->ExtendedBuildingPlacing)
		{
			R->EDX<BlitterFlags>(flags | (zero ? BlitterFlags::Zero : BlitterFlags::Nonzero));
			return DrawVanillaAlt;
		}
		else if (BuildingTypeClass* const pType = abstract_cast<BuildingTypeClass*>(DisplayClass::Instance.CurrentBuildingTypeCopy))
		{
			R->Stack<bool>(STACK_OFFSET(0x30, -0x1D), pCell->CanThisExistHere(pType->SpeedType, pType, HouseClass::CurrentPlayer));
			R->EDX<BlitterFlags>(flags | BlitterFlags::TransLucent75);
			return DontDrawAlt;
		}
	}

	R->EDX<BlitterFlags>(flags | (zero ? BlitterFlags::Zero : BlitterFlags::Nonzero));
	return DontDrawAlt;
}

/*
	In sub_47EC90

	- Buildable-upon TechnoTypes Hook #2-2 -> Draw different color grid
*/
DEFINE_HOOK(0x47EF52, CellClass_DrawPlaceGrid_DrawGrids, 0x6)
{
	const auto pRules = RulesExt::Global();

	if (!pRules->PlacementGrid_Expand)
		return 0;

	const auto pCell = ProximityTemp::CurrentCell;
	const auto cell = pCell->MapCoords;
	const auto pObj = DisplayClass::Instance.CurrentBuildingType;
	const auto range = static_cast<short>((pObj && pObj->WhatAmI() == AbstractType::BuildingType) ? static_cast<BuildingTypeClass*>(pObj)->Adjacent + 1 : 0);

	const auto maxX = static_cast<short>(cell.X + range);
	const auto maxY = static_cast<short>(cell.Y + range);
	const auto minX = static_cast<short>(cell.X - range);
	const auto minY = static_cast<short>(cell.Y - range);

	bool green = false;
	// Brute force
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

/*
	In sub_42EB50

	- Limbo Build Hook #4 -> Check Base Node
	- AIConstructionYard Hook #4-2 -> Prohibit AI from building construction yard
*/
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

	return pBase->Owner->IsValidBaseNode(pBaseNode) ? Valid : Invalid;
}

/*
	In sub_45C300

	- Fix foundation outside Hook -> Fix incorrect building type's FoundationOutside
*/
DEFINE_HOOK(0x45DD04, BuildingTypeClass_InitFoundationOutsides, 0x5)
{
	auto& foundationOutsides = Make_Global<CellStruct[22][30]>(0x89D368);
	constexpr auto CellStruct_OutsideEnd = CellStruct { 0x7FFF, 0x7FFF };

	// 4x3 remove incorrect (3,2), (3,1), (3,0) and add missing (4,3), (4,2), (4,1), (4,0), (4,-1)
	{
		auto& outside4x3 = foundationOutsides[12];
		outside4x3[6] = CellStruct { 4, 2 };
		outside4x3[8] = CellStruct { 4, 1 };
		outside4x3[10] = CellStruct { 4, 0 };
		outside4x3[16] = CellStruct { 4, -1 };
		outside4x3[17] = CellStruct { 4, 3 };
		outside4x3[18] = CellStruct_OutsideEnd;
	}

	// 2x6 add missing (-1,4) and (2,4)
	{
		auto& outside2x6 = foundationOutsides[15];
		outside2x6[18] = CellStruct { -1, 4 };
		outside2x6[19] = CellStruct { 2, 4 };
		outside2x6[20] = CellStruct_OutsideEnd;
	}

	// 2x5 add missing (-1,4) and (2,4)
	{
		auto& outside2x5 = foundationOutsides[16];
		outside2x5[16] = CellStruct { -1, 4 };
		outside2x5[17] = CellStruct { 2, 4 };
		outside2x5[18] = CellStruct_OutsideEnd;
	}

	// 4x4 remove duplicate (-1,4) and (4,4)
	{
		auto& outside4x4 = foundationOutsides[18];
		outside4x4[16] = CellStruct { 4, 3 };
		outside4x4[20] = CellStruct_OutsideEnd;
		outside4x4[21] = CellStruct::Empty;
		outside4x4[22] = CellStruct::Empty;
	}

	// 3x4 add missing (-1,3) and (3,3)
	{
		auto& outside3x4 = foundationOutsides[19];
		outside3x4[16] = CellStruct { -1, 3 };
		outside3x4[17] = CellStruct { 3, 3 };
		outside3x4[18] = CellStruct_OutsideEnd;
	}

	// 6x4 fix the entire error without (2,-1)
	{
		auto& outside6x4 = foundationOutsides[20];
		outside6x4[1] = CellStruct { -1, -1 };
		outside6x4[2] = CellStruct { 0, -1 };
		outside6x4[3] = CellStruct { 1, -1 };
		outside6x4[4] = CellStruct { 3, -1 };
		outside6x4[5] = CellStruct { 4, -1 };
		outside6x4[6] = CellStruct { 5, -1 };
		outside6x4[7] = CellStruct { 6, -1 };
		outside6x4[8] = CellStruct { -1, 0 };
		outside6x4[9] = CellStruct { 6, 0 };
		outside6x4[10] = CellStruct { -1, 1 };
		outside6x4[11] = CellStruct { 6, 1 };
		outside6x4[12] = CellStruct { -1, 2 };
		outside6x4[13] = CellStruct { 6, 2 };
		outside6x4[14] = CellStruct { -1, 3 };
		outside6x4[15] = CellStruct { 6, 3 };
		outside6x4[16] = CellStruct { -1, 4 };
		outside6x4[17] = CellStruct { 0, 4 };
		outside6x4[18] = CellStruct { 1, 4 };
		outside6x4[19] = CellStruct { 2, 4 };
		outside6x4[20] = CellStruct { 3, 4 };
		outside6x4[21] = CellStruct { 4, 4 };
		outside6x4[22] = CellStruct { 5, 4 };
		outside6x4[23] = CellStruct { 6, 4 };
		outside6x4[24] = CellStruct_OutsideEnd;
	}

	return 0;
}
