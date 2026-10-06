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

#pragma region RallyPointEnhancement

DEFINE_HOOK(0x44368D, BuildingClass_ObjectClickedAction_RallyPoint, 0x7)
{
	enum { OnTechno = 0x44363C };

	return RulesExt::Global()->RallyPointIgnoreReachability ? OnTechno : 0;
}

DEFINE_HOOK(0x4473F4, BuildingClass_MouseOverObject_JustHasRallyPoint, 0x6)
{
	enum { SkipFactoryCheck = 0x447413, SkipAllCheck = 0x44752C };

	GET(BuildingClass* const, pThis, ESI);

	if (RulesExt::Global()->RallyPointIgnoreReachability)
		return SkipAllCheck;

	return BuildingTypeExt::Fetch(pThis->Type)->JustHasRallyPoint ? SkipFactoryCheck : 0;
}

DEFINE_HOOK(0x447643, BuildingClass_MouseOverCell_IgnoreReachability, 0x5)
{
	enum { SkipGameCode = 0x44774B };

	return RulesExt::Global()->RallyPointIgnoreReachability ? SkipGameCode : 0;
}

DEFINE_HOOK(0x44398C, BuildingClass_SetRallyPoint_IgnoreReachability, 0x5)
{
	GET_STACK(CellStruct*, pTarget, STACK_OFFSET(0xA4, 0x4));

	if (RulesExt::Global()->RallyPointIgnoreReachability)
		R->EAX(MapClass::Instance.GetCellAt(*pTarget));

	return 0;
}

DEFINE_HOOK(0x70000E, TechnoClass_MouseOverObject_RallyPointIgnoreReachability, 0x5)
{
	enum { AlwaysAlt = 0x700038 };

	GET(TechnoClass* const, pThis, ESI);

	if (pThis->WhatAmI() == AbstractType::Building && RulesExt::Global()->RallyPointIgnoreReachability)
	{
		auto const pType = static_cast<BuildingClass*>(pThis)->Type;
		auto const pTypeExt = BuildingTypeExt::Fetch(pType);
		bool HasRallyPoint = pTypeExt->JustHasRallyPoint || pType->Factory == AbstractType::UnitType || pType->Factory == AbstractType::InfantryType || pType->Factory == AbstractType::AircraftType;
		return HasRallyPoint ? AlwaysAlt : 0;
	}

	return 0;
}

DEFINE_HOOK(0x44748E, BuildingClass_MouseOverObject_JustHasRallyPointAircraft, 0x6)
{
	enum { JustRally = 0x44749D };

	GET(BuildingClass* const, pThis, ESI);

	return BuildingTypeExt::Fetch(pThis->Type)->JustHasRallyPoint ? JustRally : 0;
}

DEFINE_HOOK(0x447674, BuildingClass_MouseOverCell_JustHasRallyPoint1, 0x6)
{
	enum { JustRally = 0x447683 };

	GET(BuildingClass* const, pThis, ESI);

	return BuildingTypeExt::Fetch(pThis->Type)->JustHasRallyPoint ? JustRally : 0;
}

DEFINE_HOOK(0x447643, BuildingClass_MouseOverCell_JustHasRallyPoint2, 0x5)
{
	enum { JustRally = 0x447674 };

	GET(BuildingClass* const, pThis, ESI);

	return BuildingTypeExt::Fetch(pThis->Type)->JustHasRallyPoint ? JustRally : 0;
}

DEFINE_HOOK(0x700B28, TechnoClass_MouseOverCell_JustHasRallyPoint, 0x6)
{
	enum { JustRally = 0x700B30 };

	GET(TechnoClass* const, pThis, ESI);

	if (pThis->WhatAmI() == AbstractType::Building)
		return BuildingTypeExt::Fetch(static_cast<BuildingClass*>(pThis)->Type)->JustHasRallyPoint ? JustRally : 0;

	return 0;
}

DEFINE_HOOK(0x455DA0, BuildingClass_IsUnitFactory_JustHasRallyPoint, 0x6)
{
	enum { SkipGameCode = 0x455DCD };

	GET(BuildingClass* const, pThis, ECX);

	return BuildingTypeExt::Fetch(pThis->Type)->JustHasRallyPoint ? SkipGameCode : 0;
}

// Handle the rally of infantry.
DEFINE_HOOK(0x444CA3, BuildingClass_KickOutUnit_RallyPointAreaGuard1, 0x6)
{
	enum { SkipQueueMove = 0x444D11 };

	GET(BuildingClass*, pThis, ESI);
	GET(FootClass*, pProduct, EDI);

	if (!pThis->Owner->IsControlledByHuman() || !pProduct->Owner->IsControlledByHuman())
		return 0;

	if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetArchiveTarget(pThis->ArchiveTarget);
		pProduct->SetDestination(pThis->ArchiveTarget, true);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return 0;
}

// Vehicle but without BuildingClass::Unload calling, e.g. the building has WeaponsFactory = no set.
// Also fix the bug that WeaponsFactory = no will make the product ignore the rally point.
// Also fix the bug that WeaponsFactory = no will make the Jumpjet product park on the ground.
DEFINE_HOOK(0x4448CE, BuildingClass_KickOutUnit_RallyPointAreaGuard2, 0x6)
{
	enum { SkipGameCode = 0x4448F8 };

	GET(FootClass*, pProduct, EDI);
	GET(BuildingClass*, pThis, ESI);

	if (!pThis->Owner->IsControlledByHuman() || !pProduct->Owner->IsControlledByHuman())
		return 0;

	auto const pFocus = pThis->ArchiveTarget;
	auto const pUnit = abstract_cast<UnitClass*, true>(pProduct);
	bool isHarvester = pUnit ? pUnit->Type->Harvester : false;

	if (isHarvester)
	{
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Harvest, true);
	}
	else if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetArchiveTarget(pFocus);
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Area_Guard, true);
	}
	else
	{
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Move, true);
	}

	if (!pFocus && pProduct->GetTechnoType()->Locomotor == LocomotionClass::CLSIDs::Jumpjet)
		pProduct->Scatter(CoordStruct::Empty, false, false);

	return SkipGameCode;
}

// This makes the building has WeaponsFactory = no to kick out units in the cell same as WeaponsFactory = yes.
// Also enhanced the ExitCoord.
DEFINE_HOOK(0x4448B0, BuildingClass_KickOutUnit_ExitCoords, 0x6)
{
	GET(FootClass*, pProduct, EDI);
	GET(BuildingClass*, pThis, ESI);
	GET(CoordStruct*, pCrd, ECX);
	REF_STACK(DirType, dir, STACK_OFFSET(0x144,-0x100));

	auto const pProductType = pProduct->GetTechnoType();
	auto const isJJ = pProductType->Locomotor == LocomotionClass::CLSIDs::Jumpjet;
	auto const buildingExitCrd = isJJ ? BuildingTypeExt::Fetch(pThis->Type)->JumpjetExitCoord.Get(pThis->Type->ExitCoord)
		: TechnoExt::Fetch(pThis)->TypeExtData->ExitCoord.Get(pThis->Type->ExitCoord);
	auto const exitCrd = TechnoTypeExt::Fetch(pProductType)->ExitCoord.Get(buildingExitCrd);

	pCrd->X += exitCrd.X;
	pCrd->Y += exitCrd.Y;
	pCrd->Z += exitCrd.Z;

	if (!isJJ)
	{
		auto nCell = CellClass::Coord2Cell(*pCrd);
		auto const pCell = MapClass::Instance.GetCellAt(nCell);
		bool isBridge = pCell->ContainsBridge();
		nCell = MapClass::Instance.NearByLocation(CellClass::Coord2Cell(*pCrd),
			pProductType->SpeedType, -1, pProductType->MovementZone, isBridge, 1, 1, false,
			false, false, isBridge, nCell, false, false);
		*pCrd = CellClass::Cell2Coord(nCell, pCrd->Z);
	}

	dir = DirType::East;
	return 0;
}

// Ships.
DEFINE_HOOK(0x444424, BuildingClass_KickOutUnit_RallyPointAreaGuard3, 0x5)
{
	enum { SkipQueueMove = 0x44443F };

	GET(FootClass*, pProduct, EDI);
	GET(AbstractClass*, pFocus, ESI);

	if (!pProduct->Owner->IsControlledByHuman())
		return 0;

	auto const pUnit = abstract_cast<UnitClass*, true>(pProduct);
	const bool isHarvester = pUnit ? pUnit->Type->Harvester : false;

	if (RulesExt::Global()->RallyPointAreaGuard && !isHarvester)
	{
		pProduct->SetArchiveTarget(pFocus);
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return 0;
}

// For common aircrafts.
// Also make AirportBound aircraft not ignore the rally point.
DEFINE_HOOK(0x444061, BuildingClass_KickOutUnit_RallyPointAreaGuard4, 0x6)
{
	enum { SkipQueueMove = 0x444091, NotSkip = 0x444075 };

	GET(FootClass*, pProduct, EBP);
	GET(AbstractClass*, pFocus, ESI);

	if (!pProduct->Owner->IsControlledByHuman())
		return 0;

	if (TechnoExt::Fetch(pProduct)->TypeExtData->IgnoreRallyPoint)
		return SkipQueueMove;

	if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetArchiveTarget(pFocus);
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return NotSkip;
}

// For aircrafts with AirportBound = no and the airport is full.
// Still some other bug in it.
DEFINE_HOOK(0x443EB8, BuildingClass_KickOutUnit_RallyPointAreaGuard5, 0x5)
{
	enum { SkipQueueMove = 0x443ED3 };

	GET(FootClass*, pProduct, EBP);
	GET(AbstractClass*, pFocus, EAX);

	if (!pProduct->Owner->IsControlledByHuman())
		return 0;

	if (TechnoExt::Fetch(pProduct)->TypeExtData->IgnoreRallyPoint)
		return SkipQueueMove;

	if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetArchiveTarget(pFocus);
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return 0;
}

// For unloaded units.
DEFINE_HOOK(0x73AAB3, UnitClass_UpdateMoving_RallyPointAreaGuard, 0x5)
{
	enum { SkipQueueMove = 0x73AAC1 };

	GET(UnitClass*, pThis, EBP);
	GET(AbstractClass*, pFocus, EAX);

	if (!pThis->Owner->IsControlledByHuman())
		return 0;

	if (RulesExt::Global()->RallyPointAreaGuard && !pThis->Type->Harvester)
	{
		pThis->SetArchiveTarget(pFocus);
		pThis->SetDestination(pFocus, true);
		pThis->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return 0;
}

DEFINE_HOOK(0x4438C9, BuildingClass_SetRallyPoint_PathFinding, 0x6)
{
	GET(BuildingClass* const, pThis, EBP);
	GET(int, movementzone, ESI);
	GET_STACK(int, speedtype, STACK_OFFSET(0xA4, -0x84));

	auto const pExt = BuildingTypeExt::Fetch(pThis->Type);
	R->ESI(pExt->RallyMovementZone.Get(movementzone));
	R->Stack(STACK_OFFSET(0xA4, -0x84), pExt->RallySpeedType.Get(speedtype));

	return 0;
}

#pragma endregion
