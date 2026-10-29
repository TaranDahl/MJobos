#include <IsometricTileTypeClass.h>

#include "Body.h"

#include <Ext/BuildingType/Body.h>
#include <Ext/BuildingType/ProximityTemp.h>
#include <Ext/TerrainType/Body.h>
#include <Ext/TechnoType/Body.h>
#include <Ext/CaptureManager/Body.h>
#include <Ext/Building/Body.h>

#pragma region AllowDeployControlledMCV

DEFINE_HOOK_AGAIN(0x443770, TechnoClass_AllowDeployControlledMCV, 0x6)// BuildingClass::CellClickedAction
DEFINE_HOOK_AGAIN(0x443AB0, TechnoClass_AllowDeployControlledMCV, 0x6)// BuildingClass::SetRallyPoint
DEFINE_HOOK_AGAIN(0x44F614, TechnoClass_AllowDeployControlledMCV, 0x6)// BuildingClass::IsControllable
DEFINE_HOOK(0x700ED0, TechnoClass_AllowDeployControlledMCV, 0x6)// UnitClass::CanDeploySlashUnload
{
	return RulesExt::Global()->AllowDeployControlledMCV ? R->Origin() + 0xE : 0;
}

#pragma endregion

static inline void TransferMindControlOnDeploy(TechnoClass* pTechnoFrom, TechnoClass* pTechnoTo)
{
	const auto pAnimType = pTechnoFrom->MindControlRingAnim
		? pTechnoFrom->MindControlRingAnim->Type
		: TechnoExt::Fetch(pTechnoFrom)->MindControlRingAnimType;

	if (const auto Controller = pTechnoFrom->MindControlledBy)
	{
		if (const auto Manager = Controller->CaptureManager)
		{
			CaptureManagerExt::FreeUnit(Manager, pTechnoFrom, true);

			if (CaptureManagerExt::CaptureUnit(Manager, pTechnoTo, false, pAnimType, true))
			{
				if (const auto pBld = abstract_cast<BuildingClass*, true>(pTechnoTo))
				{
					// Capturing the building after unlimbo before buildup has finished or even started appears to throw certain things off,
					// Hopefully this is enough to fix most of it like anims playing prematurely etc.
					pBld->ActuallyPlacedOnMap = false;
					pBld->DestroyNthAnim(BuildingAnimSlot::All);

					pBld->BeginMode(BStateType::Construction);
					pBld->QueueMission(Mission::Construction, false);
				}
			}
			else
			{
				int nSound = pTechnoTo->GetTechnoType()->MindClearedSound;
				if (nSound == -1)
					nSound = RulesClass::Instance->MindClearedSound;

				if (nSound != -1)
					VocClass::PlayIndexAtPos(nSound, pTechnoTo->Location);
			}
		}
	}
	else if (const auto MCHouse = pTechnoFrom->MindControlledByHouse)
	{
		pTechnoTo->MindControlledByHouse = MCHouse;
		pTechnoFrom->MindControlledByHouse = nullptr;
	}
	else if (pTechnoFrom->MindControlledByAUnit) // Perma MC
	{
		pTechnoTo->MindControlledByAUnit = true;

		const auto pBuilding = abstract_cast<BuildingClass*, true>(pTechnoTo);
		CoordStruct location = pTechnoTo->GetCoords();

		location.Z += pBuilding
			? pBuilding->Type->Height * Unsorted::LevelHeight
			: pTechnoTo->GetTechnoType()->MindControlRingOffset;

		const auto pAnim = pAnimType
			? GameCreate<AnimClass>(pAnimType, location, 0, 1)
			: nullptr;

		if (pAnim)
		{
			if (pBuilding)
				pAnim->ZAdjust = -1024;

			pTechnoTo->MindControlRingAnim = pAnim;
			pAnim->SetOwnerObject(pTechnoTo);
		}
	}
}

DEFINE_HOOK(0x739956, UnitClass_Deploy_Transfer, 0x6)
{
	GET(UnitClass*, pUnit, EBP);
	GET(BuildingClass*, pStructure, EBX);

	TransferMindControlOnDeploy(pUnit, pStructure);
	ShieldClass::SyncShieldToAnother(pUnit, pStructure);
	TechnoExt::SyncInvulnerability(pUnit, pStructure);
	AttachEffectClass::TransferAttachedEffects(pUnit, pStructure);

	return 0;
}

DEFINE_HOOK(0x44A03C, BuildingClass_Mi_Selling_Transfer, 0x6)
{
	GET(BuildingClass*, pStructure, EBP);
	GET(UnitClass*, pUnit, EBX);

	TransferMindControlOnDeploy(pStructure, pUnit);
	ShieldClass::SyncShieldToAnother(pStructure, pUnit);
	TechnoExt::SyncInvulnerability(pStructure, pUnit);
	AttachEffectClass::TransferAttachedEffects(pStructure, pUnit);

	// This line will break the bahavior of UnDeploysInto buildings. However, it might serve a purpose that no one knows yet
	// Comment out the line instead of removing it for now, so we can turn to it if something related goes wrong in the future
	// pUnit->QueueMission(Mission::Hunt, true);
	return 0;
}

DEFINE_HOOK(0x449E2E, BuildingClass_Mi_Selling_CreateUnit, 0x6)
{
	GET(BuildingClass*, pStructure, EBP);
	R->ECX<HouseClass*>(pStructure->GetOriginalOwner());

	// Remember MC ring animation.
	if (pStructure->IsMindControlled())
	{
		auto const pTechnoExt = TechnoExt::Fetch(pStructure);
		pTechnoExt->UpdateMindControlAnim();
	}

	return 0x449E34;
}

DEFINE_HOOK(0x7396AD, UnitClass_Deploy_CreateBuilding, 0x6)
{
	GET(UnitClass*, pUnit, EBP);
	R->EDX<HouseClass*>(pUnit->GetOriginalOwner());

	return 0x7396B3;
}

// Game removes deploying vehicles from map temporarily to check if there's enough
// space to deploy into a building when displaying allow/disallow deploy cursor.
// This can cause desyncs if there are certain types of units around the deploying
// unit because the OccupationFlags may be accidentally cleared, or the order of
// the objects linked list may be scrambled.
#pragma region DeploysIntoDesyncFix

DEFINE_HOOK(0x73FEC1, UnitClass_WhatAction_DeploysIntoDesyncFix, 0x6)
{
	enum { SkipGameCode = 0x73FFDF };

	GET(UnitClass* const, pThis, ESI);
	REF_STACK(Action, action, STACK_OFFSET(0x20, 0x8));

	if (!UnitExt::CanDeployIntoBuilding(pThis))
		action = Action::NoDeploy;

	return SkipGameCode;
}

// Exclude the specific unit who want to deploy
// Allow placing buildings on top of TerrainType with CanBeBuiltOn
static inline void MarkPlacementGridOccupier(ObjectClass* const pObject, HouseClass* const pOwner)
{
	const auto absType = pObject->WhatAmI();

	if (!RulesExt::Global()->PlacementGrid_Expand
		|| pObject == UnitExt::Deployer
		|| (absType != AbstractType::Infantry && absType != AbstractType::Unit))
	{
		return;
	}

	const auto pTechno = static_cast<TechnoClass*>(pObject);

	if (pTechno->GetTechnoType()->Speed > 0 && BuildingTypeExt::CheckOccupierCanLeave(pOwner, pTechno->Owner))
		ProximityTemp::Exist = true;
}

// Allow placing buildings on top of TechnoTypes with CanBeBuiltOn or under units
static inline bool CheckCanNotExistHere(FootClass* const pTechno, bool canBuildUnderUnits,
	bool& skipFlag, bool& builtOnCanBeBuiltOn)
{
	if (pTechno == UnitExt::Deployer)
	{
		skipFlag = true;
		return false;
	}

	if (canBuildUnderUnits || TechnoTypeExt::Fetch(pTechno->GetTechnoType())->CanBeBuiltOn)
	{
		builtOnCanBeBuiltOn = true;
		return false;
	}

	return true;
}

DEFINE_HOOK(0x47C640, CellClass_CanThisExistHere_IgnoreSomething, 0x6)
{
	enum { CanNotExistHere = 0x47C6D1, CanExistHere = 0x47C6A0 };

	GET(const CellClass* const, pCell, EDI);
	GET(const BuildingTypeClass* const, pBuildingType, EAX);
	GET_STACK(HouseClass* const, pOwner, STACK_OFFSET(0x18, 0xC));

	ProximityTemp::Exist = false;

	if (!Game::IsActive)
		return CanExistHere;

	const bool canBuildUnderUnits = BuildingTypeExt::Fetch(pBuildingType)->CanBuildUnderUnits.Get();

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
					if (!TechnoTypeExt::Fetch(pObject->GetTechnoType())->CanBeBuiltOn)
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
					break;
			}
		}
	}
	else if (pBuildingType->LaserFencePost || pBuildingType->Gate)
	{
		bool skipFlag = UnitExt::Deployer ? UnitExt::Deployer->CurrentMapCoords == pCell->MapCoords : false;
		bool builtOnCanBeBuiltOn = false;

		for (auto pObject = pCell->FirstObject; pObject; pObject = pObject->NextObject)
		{
			MarkPlacementGridOccupier(pObject, pOwner);

			switch (pObject->WhatAmI())
			{
				case AbstractType::Aircraft:
				{
					if (!canBuildUnderUnits && !TechnoTypeExt::Fetch(pObject->GetTechnoType())->CanBeBuiltOn)
						return CanNotExistHere;

					builtOnCanBeBuiltOn = true;
					break;
				}

				case AbstractType::Building:
				{
					const auto pBuilding = static_cast<BuildingClass*>(pObject);
					const auto pType = pBuilding->Type;

					if (TechnoTypeExt::Fetch(pType)->CanBeBuiltOn)
					{
						builtOnCanBeBuiltOn = true;
					}
					else if (pOwner != pBuilding->Owner || !pType->LaserFence)
					{
						return CanNotExistHere;
					}

					break;
				}

				case AbstractType::Infantry:
				case AbstractType::Unit:
				{
					if (CheckCanNotExistHere(static_cast<FootClass*>(pObject), canBuildUnderUnits,
						skipFlag, builtOnCanBeBuiltOn))
					{
						return CanNotExistHere;
					}

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
					break;
			}
		}

		if (!builtOnCanBeBuiltOn && (pCell->OccupationFlags & (skipFlag ? 0x1F : 0x3F)))
			return CanNotExistHere;
	}
	else if (pBuildingType->ToTile)
	{
		const auto isoTileTypeIndex = pCell->IsoTileTypeIndex;

		if (isoTileTypeIndex >= 0 && isoTileTypeIndex < IsometricTileTypeClass::Array.Count
			&& !IsometricTileTypeClass::Array.Items[isoTileTypeIndex]->Morphable)
		{
			return CanNotExistHere;
		}

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
			MarkPlacementGridOccupier(pObject, pOwner);

			switch (pObject->WhatAmI())
			{
				case AbstractType::Aircraft:
				{
					if (!canBuildUnderUnits && !TechnoTypeExt::Fetch(pObject->GetTechnoType())->CanBeBuiltOn)
						return CanNotExistHere;

					builtOnCanBeBuiltOn = true;
					break;
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
					if (CheckCanNotExistHere(static_cast<FootClass*>(pObject), canBuildUnderUnits,
						skipFlag, builtOnCanBeBuiltOn))
					{
						return CanNotExistHere;
					}

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
					break;
			}
		}

		if (!builtOnCanBeBuiltOn && (pCell->OccupationFlags & (skipFlag ? 0x1F : 0x3F)))
			return CanNotExistHere;
	}

	return CanExistHere; // Continue check the overlays .etc
}


DEFINE_HOOK(0x7396D2, UnitClass_TryToDeploy_Transfer, 0x5)
{
	GET(UnitClass*, pUnit, EBP);
	GET(BuildingClass*, pStructure, EBX);

	if (pUnit->Type->DeployToFire && pUnit->Target)
		pStructure->LastTarget = pUnit->Target;

	const auto pStructureExt = BuildingExt::Fetch(pStructure);
	pStructureExt->DeployedTechno = true;

	return 0;
}

#pragma endregion
