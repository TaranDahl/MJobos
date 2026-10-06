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

#pragma region RocketUpdateTargetPosition

DEFINE_HOOK(0x6B75AC, SpawnManagerClass_AI_SetDestinationForMissiles, 0x5)
{
	enum { QueueMoveMission = 0x6B75BC };

	GET(SpawnManagerClass*, pSpawnManager, ESI);
	GET(AircraftClass*, pSpawnee, EDI);

	const auto pTarget = pSpawnManager->Target;
	pSpawnee->SetDestination(pTarget, true);

	if (const auto pLoco = locomotion_cast<RocketLocomotionClass*>(pSpawnee->Locomotor))
	{
		if (const auto pTargetObject = abstract_cast<ObjectClass*>(pTarget))
			pLoco->MovingDestination = pTargetObject->GetTargetCoords();
	}

	return QueueMoveMission;
}

DEFINE_HOOK(0x662957, RocketLocomotionClass_Process_UpdateTargetPositionWhenBoost, 0x5)
{
	enum { CheckHeight = 0x662962 };

	GET(ILocomotion*, pThis, ESI);

	const auto pLoco = static_cast<RocketLocomotionClass*>(pThis);
	const auto pRocket = static_cast<AircraftClass*>(pLoco->LinkedTo);
	const bool tracing = AircraftExt::Fetch(pRocket)->GetTypeExtData()->Missile_Tracing;
	if (tracing)
	{
		if (const auto pTarget = abstract_cast<FootClass*>(pRocket->Target))
			pLoco->MovingDestination = pTarget->GetCoords();
	}

	int heightTarget = pRocket->Location.Z - pLoco->MovingDestination.Z;
	if (MapClass::Instance.GetCellAt(pLoco->MovingDestination)->ContainsBridge())
		heightTarget -= CellClass::BridgeHeight;

	const int checkHeight = Math::min(pRocket->GetHeight(), heightTarget);

	if (tracing)
	{
		GET(RocketStruct*, pRocketStruct, EDI);

		if (!pRocketStruct->LazyCurve && checkHeight >= (pRocketStruct->Altitude / 4))
			pRocket->PrimaryFacing.SetDesired(DirStruct(Math::atan2(pRocket->Location.Y - pLoco->MovingDestination.Y, pLoco->MovingDestination.X - pRocket->Location.X)));
	}

	R->EAX(checkHeight);
	return CheckHeight;
}

DEFINE_HOOK(0x662A1E, RocketLocomotionClass_Process_UpdateTargetPositionWhenCruise, 0x5)
{
	GET(ILocomotion*, pThis, ESI);

	const auto pLoco = static_cast<RocketLocomotionClass*>(pThis);
	const auto pRocket = static_cast<AircraftClass*>(pLoco->LinkedTo);
	if (AircraftExt::Fetch(pRocket)->GetTypeExtData()->Missile_Tracing)
	{
		if (const auto pTarget = abstract_cast<FootClass*>(pRocket->Target))
			pLoco->MovingDestination = pTarget->GetCoords();
	}

	return 0;
}

DEFINE_HOOK(0x662CDF, RocketLocomotionClass_Process_UpdateTargetPositionWhenDive, 0x5)
{
	GET(ILocomotion*, pThis, ESI);

	const auto pLoco = static_cast<RocketLocomotionClass*>(pThis);
	const auto pRocket = static_cast<AircraftClass*>(pLoco->LinkedTo);
	if (AircraftExt::Fetch(pRocket)->GetTypeExtData()->Missile_Tracing)
	{
		if (const auto pTarget = abstract_cast<FootClass*>(pRocket->Target))
			pLoco->MovingDestination = pTarget->GetCoords();

		GET(int, dX, EAX);
		GET(int, dY, ECX);
		pRocket->PrimaryFacing.SetDesired(DirStruct(Math::atan2(-dY, dX)));
	}

	return 0;
}

DEFINE_HOOK(0x54E42B, Kamikaze_Add_SetTarget, 0x6)
{
	enum { SetKamikazeTarget = 0x54E475 };

	GET(AbstractClass*, pTarget, ECX);

	R->EAX(pTarget);

	return SetKamikazeTarget;
}

DEFINE_HOOK(0x54E60A, Kamikaze_Remove_ResetTarget, 0x6)
{
	enum { ContinueLoop = 0x54E5B7 };

	GET(AbstractClass*, pInvalidTarget, ECX);
	GET(Kamikaze*, pKamikaze, EDI);
	GET(int, index, ESI);

	auto getNewTarget = [pInvalidTarget]() -> CellClass*
	{
		if (const auto pInvalidTargetBuilding = abstract_cast<BuildingClass*>(pInvalidTarget))
		{
			if (pInvalidTargetBuilding->Type)
				return MapClass::Instance.GetCellAt(pInvalidTargetBuilding->GetTargetCoords());
		}
		return MapClass::Instance.GetCellAt(pInvalidTarget->GetCoords());
	};
	const auto pNewTarget = getNewTarget();
	const auto pControl = pKamikaze->Nodes.Items[index];
	const auto pAircraft = pControl->Item;
	pAircraft->SetTarget(pNewTarget);
	pAircraft->QueueMission(Mission::Attack, false);
	pControl->Target = pNewTarget;

	return ContinueLoop;
}

DEFINE_HOOK(0x41AA91, AircraftClass_SetDestination_SetRocketDestination, 0x7)
{
	enum { ContinueCheckLanding = 0x41AAB1, SetNoDestination = 0x41AA9C };

	GET(AircraftClass*, pThis, ESI);
	GET(AbstractClass*, pDest, EDI);

	return pDest->IsInAir() && (!pThis->IsALoaner || !pThis->Type->MissileSpawn || !pThis->SpawnOwner) ? SetNoDestination : ContinueCheckLanding;
}

#pragma endregion

#pragma region HardLoco
/*
DEFINE_HOOK_AGAIN(0x742A8C, UnitClass_SetDestination_PiggyBack, 0x8)
DEFINE_HOOK(0x742691, UnitClass_SetDestination_PiggyBack, 0x8)
{
	REF_STACK(const _GUID*, ID, STACK_OFFSET(0x9C, -0x9C));
	ID = &__uuidof(AdvancedDriveLocomotionClass);//&LocomotionClass::CLSIDs::Jumpjet;
	return 0;
}
*/
#pragma endregion

#pragma region NewFactories

// Disappear
// 0x44EC3A
// BeginProduction
// 0x4FA39C
// 0x4FA553
// 0x4FA76D
// SuspendProduction
// 0x4FA942
// AbandonProduction
// 0x4FAA5C
// 0x4FABCB
// UnitFromFactory
// 0x4FB11D
// PointerExpired
// 0x4FBC75
// GetPrimaryFactory
// 0x500510
// SetPrimaryFactory
// 0x500850
// UpdateFactoriesQueues
// 0x509149
// ShouldDisableCameo
// 0x50B3A0

// FindFactory -> Ares hooks all of these away
// 0x5F7900

#pragma endregion

#pragma region PlanWaypoint

DEFINE_HOOK(0x63745D, UnknownClass_PlanWaypoint_ContinuePlanningOnEnter, 0x6)
{
	enum { SkipDeselect = 0x637468 };

	GET(const int, planResult, ESI);

	return (!planResult && !RulesExt::Global()->StopPlanningOnEnter) ? SkipDeselect : 0;
}

DEFINE_HOOK(0x637479, UnknownClass_PlanWaypoint_DisableMessage, 0x5)
{
	enum { SkipMessage = 0x637524 };
	return (!RulesExt::Global()->StopPlanningOnEnter) ? SkipMessage : 0;
}

DEFINE_HOOK(0x638D73, UnknownClass_CheckLastWaypoint_ContinuePlanningWaypoint, 0x5)
{
	enum { SkipDeselect = 0x638D8D, Deselect = 0x638D82 };

	GET(const Action, action, EAX);

	if (!RulesExt::Global()->StopPlanningOnEnter)
		return SkipDeselect;
	else if (action == Action::Select || action == Action::ToggleSelect || action == Action::Capture)
		return Deselect;

	return SkipDeselect;
}

#pragma endregion

#pragma region KeepTemporal

DEFINE_HOOK(0x6F50A9, TechnoClass_UpdatePosition_TemporalLetGo, 0x7)
{
	enum { LetGo = 0x6F50B4, SkipLetGo = 0x6F50B9 };

	GET(TechnoClass* const, pThis, ESI);
	GET(TemporalClass* const, pTemporal, ECX);

	return pTemporal && pTemporal->Target && !TechnoExt::Fetch(pThis)->TypeExtData->KeepWarping ? LetGo : SkipLetGo;
}

DEFINE_HOOK(0x709A43, TechnoClass_EnterIdleMode_TemporalLetGo, 0x7)
{
	enum { LetGo = 0x709A54, SkipLetGo = 0x709A59 };

	GET(TechnoClass* const, pThis, ESI);
	GET(TemporalClass* const, pTemporal, ECX);

	return pTemporal && pTemporal->Target && !TechnoExt::Fetch(pThis)->TypeExtData->KeepWarping ? LetGo : SkipLetGo;
}

// This is a fix to KeepWarping.
// But I think it has no difference with vanilla behavior, so no check for KeepWarping.
DEFINE_HOOK(0x4C7643, EventClass_RespondToEvent_StopTemporal, 0x6)
{
	GET(TechnoClass*, pTechno, ESI);

	auto const pTemporal = pTechno->TemporalImUsing;

	if (pTemporal && pTemporal->Target)
		pTemporal->LetGo();

	return 0;
}

DEFINE_HOOK(0x71A7A8, TemporalClass_Update_CheckRange, 0x6)
{
	enum { DontCheckRange = 0x71A84E, CheckRange = 0x71A7B4 };

	GET(TechnoClass*, pTechno, EAX);

	if (pTechno->InOpenToppedTransport)
		return CheckRange;

	return TechnoExt::Fetch(pTechno)->TypeExtData->KeepWarping ? CheckRange : DontCheckRange;
}

#pragma endregion

#pragma region MissileSpawnFLH

DEFINE_HOOK(0x6B73D2, SpawnManagerClass_Update_MissileSpawnFLH1, 0xA)
{
	GET(TechnoClass*, pOwner, ECX);

	auto pPrimary = pOwner->GetWeapon(0)->WeaponType;
	R->EAX(pPrimary->Spawner ? pPrimary : pOwner->GetWeapon(1)->WeaponType);
	return 0x6B73DE;
}

DEFINE_HOOK(0x6B73EA, SpawnManagerClass_Update_MissileSpawnFLH2, 0x5)
{
	enum { SkipCurrentBurstReset = 0x6B73FC };

	GET(SpawnManagerClass* const, pThis, ESI);
	GET(WeaponTypeClass* const, pWeaponType, EAX);
	GET(int, idx, EBX);

	auto const pSpawner = pThis->Owner;

	if (TechnoExt::Fetch(pSpawner)->TypeExtData->MissileSpawnUseOtherFLHs)
	{
		int burst = pWeaponType->Burst;
		pSpawner->CurrentBurstIndex = idx % burst;
		return SkipCurrentBurstReset;
	}

	return 0;
}

#pragma endregion

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

void __fastcall KickOutClones(const BuildingExt* const pThis, const TechnoClass* const pProduction)
{
	auto const pFactory = pThis->OwnerObject();
	auto const pFactoryType = pFactory->Type;

	if (pFactoryType->Cloning || (pFactoryType->Factory != InfantryTypeClass::AbsID && pFactoryType->Factory != UnitTypeClass::AbsID))
		return;

	auto pProductionType = pProduction->GetTechnoType();
	auto pProductionTypeExt = TechnoTypeExt::Fetch(pProductionType);

	if (!pProductionTypeExt->Cloneable)
		return;

	if (const auto clonedAs = pProductionTypeExt->ClonedAs.Get())
	{
		pProductionType = clonedAs;
		pProductionTypeExt = TechnoTypeExt::Fetch(pProductionType);
	}

	auto const pFactoryOwner = pFactory->Owner;
	auto const& pCloningSources = pProductionTypeExt->ClonedAt;
	auto kickOutClone = [pProductionType, pFactoryOwner](BuildingClass* pBuilding) -> void
	{
		auto pClone = static_cast<TechnoClass*>(pProductionType->CreateObject(pFactoryOwner));

		if (pBuilding->KickOutUnit(pClone, CellStruct::Empty) != KickOutResult::Succeeded)
			pClone->UnInit();
	};

	auto const isUnit = (pFactoryType->Factory != InfantryTypeClass::AbsID);
	// keep cloning vats for backward compat, unless explicit sources are defined
	if (!isUnit && pCloningSources.empty())
	{
		for (auto const pCloningVat : pFactoryOwner->CloningVats)
			kickOutClone(pCloningVat);
	}

	// and clone from new sources
	if (!pCloningSources.empty() || isUnit)
	{
		for (auto const pBuilding : pFactoryOwner->Buildings)
		{
			if (pBuilding->InLimbo)
				continue;

			auto const pBuildingType = pBuilding->Type;
			auto shouldClone = false;

			if (!pCloningSources.empty())
				shouldClone = pCloningSources.Contains(pBuildingType);
			else if (isUnit)
				shouldClone = BuildingTypeExt::Fetch(pBuildingType)->CloningFacility && (pBuildingType->Naval == pFactoryType->Naval);

			if (shouldClone)
				kickOutClone(pBuilding);
		}
	}
}

DEFINE_HOOK(0x4448F8, BuildingClass_KickOutUnit_CloningFacilityFix, 0x6)
{
	GET(const BuildingClass* const, pThis, ESI);
	GET(const UnitClass* const, pUnit, EDI);

	--Unsorted::ScenarioInit;
	KickOutClones(BuildingExt::Fetch(pThis), pUnit);
	++Unsorted::ScenarioInit;

	return 0;
}

#pragma endregion

#pragma region CrushBuildingOnAnyCell

namespace CrushBuildingOnAnyCell
{
	CellClass* pCell;
}

DEFINE_HOOK(0x741733, UnitClass_CrushCell_SetContext, 0x6)
{
	GET(CellClass*, pCell, ESI);

	CrushBuildingOnAnyCell::pCell = pCell;

	return 0;
}

DEFINE_HOOK(0x741925, UnitClass_CrushCell_CrushBuilding, 0x5)
{
	GET(UnitClass*, pThis, EDI);

	if (RulesExt::Global()->CrushBuildingOnAnyCell)
	{
		if (auto const pBuilding = CrushBuildingOnAnyCell::pCell->GetBuilding())
		{
			if (pBuilding->IsCrushable(pThis))
			{
				VocClass::PlayAt(pBuilding->Type->CrushSound, pThis->Location, 0);
				pBuilding->Destroy();
				pBuilding->RegisterDestruction(pThis);
				pBuilding->Mark(MarkType::Up);
				// pBuilding->Limbo(); // Vanilla do this. May be not necessary?
				pBuilding->UnInit();

				R->AL(true);
			}
		}
	}

	return 0;
}

#pragma endregion

#pragma region JumpjetSpeedType

DEFINE_HOOK(0x54B36A, JumpjetLocomotionClass_MoveTo_JumpjetSpeedType, 0x5)
{
	GET(ILocomotion* const, iloco, ESI);
	REF_STACK(SpeedType, speedType, STACK_OFFSET(0x5C, -0x54));

	__assume(iloco != nullptr);
	const auto pLoco = static_cast<JumpjetLocomotionClass*>(iloco);
	const auto pTypeExt = TechnoExt::Fetch(pLoco->LinkedTo)->TypeExtData;
	speedType = static_cast<SpeedType>(pTypeExt->JumpjetSpeedType.Get());

	return 0;
}

#pragma endregion

#pragma region UpdateReload

DEFINE_HOOK(0x51BDCF, InfantryClass_Update_Reload, 0x7)
{
	enum { SkipGameCode = 0x51BDD6 };

	GET(InfantryClass*, pThis, ESI);

	R->EAX(pThis->InWhichLayer());

	if (RulesExt::Global()->InTransportInfantryAmmoFix && AresHelper::CanUseAres && pThis->InLimbo && !TechnoTypeExt::Fetch(pThis->Type)->ReloadInTransport)
		pThis->Reload();

	return SkipGameCode;
}

#pragma endregion

#pragma region UpdateInLimbo

DEFINE_HOOK(0x522937, InfantryClass_EnterOccupyBuilding_KeepUpdate, 0xA)
{
	GET(InfantryClass*, pThis, ESI);
	GET_STACK(BuildingClass*, pBuilding, STACK_OFFSET(0x1C, 0x4));

	if (RulesExt::Global()->UpdateInLimbo_Occupier)
		LogicClass::Instance.AddObject(pThis, false);

	TechnoExt::Fetch(pThis)->BuildingOccupying = pBuilding;

	return 0;
}

DEFINE_HOOK(0x4733B6, PassengerClass_AddPassenger_KeepUpdate, 0x5)
{
	GET(InfantryClass*, pThis, ESI);

	if (RulesExt::Global()->UpdateInLimbo_NormalPassenger)
		LogicClass::Instance.AddObject(pThis, false);

	return 0;
}
/*
DEFINE_HOOK(0x4DB87E, FootClass_SetLocation_UpdatePassengerLocation, 0x6)
{
	return RulesExt::Global()->UpdateInLimbo_NormalPassenger ? 0x4DB888 : 0;
}
*/
DEFINE_HOOK(0x62A0A0, ParasiteClass_Update_UpdateParasiteLocation, 0x7)
{
	GET(ParasiteClass*, pThis, ESI);

	if (RulesExt::Global()->UpdateInLimbo_LimboLaunch)
		pThis->Owner->SetLocation(pThis->Victim->Location);

	return 0;
}

DEFINE_HOOK(0x6FF7F9, SITechnoClass_Fire_LimboLaunch, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	if (RulesExt::Global()->UpdateInLimbo_LimboLaunch)
		LogicClass::Instance.AddObject(pThis, false);

	return 0;
}

DEFINE_HOOK(0x4580B4, BuildingClass_OccupantLeaveAll_UpdateState, 0x5)
{
	GET(InfantryClass*, pOccupant, EDI);

	TechnoExt::Fetch(pOccupant)->BuildingOccupying = nullptr;

	return 0;
}

DEFINE_HOOK(0x6FC5B3, TechnoClass_GetFireError_InLimbo, 0x6)
{
	enum { Illegal = 0x6FC86A };

	GET(TechnoClass*, pThis, ESI);

	return !pThis->InLimbo || pThis->InOpenToppedTransport
		|| (RulesExt::Global()->UpdateInLimbo_Occupier && TechnoExt::Fetch(pThis)->BuildingOccupying)
		? 0 : Illegal;
}

#pragma endregion

#pragma region IgnoreByMouse

static inline bool ShouldIgnoreByMouse(ObjectClass* pObject)
{
	const auto pType = pObject->GetType();

	if (!pType)
		return true;

	if (const auto pTechno = abstract_cast<TechnoClass*, true>(pObject))
	{
		const auto pTypeExt = TechnoExt::Fetch(pTechno)->TypeExtData;
		const auto pOwner = pTechno->Owner;
		const auto defaultValue = pTypeExt->IgnoredByMouse;

		if (pOwner == HouseClass::CurrentPlayer)
			return pTypeExt->IgnoredByMouse_ToSelf.Get(defaultValue);
		else if (pOwner->IsAlliedWith(HouseClass::CurrentPlayer))
			return pTypeExt->IgnoredByMouse_ToAlly.Get(defaultValue);

		return pTypeExt->IgnoredByMouse_ToEnemy.Get(defaultValue);
	}

	const auto absType = pObject->WhatAmI();

	if (absType == OverlayClass::AbsID)
	{
		const auto pOverlay = static_cast<OverlayClass*>(pObject);
		const auto pTypeExt = OverlayTypeExt::Fetch(pOverlay->Type);
		return pTypeExt->IgnoredByMouse.Get();
	}
	else if (absType == TerrainClass::AbsID)
	{
		const auto pTerrain = static_cast<TerrainClass*>(pObject);
		const auto pTypeExt = TerrainTypeExt::Fetch(pTerrain->Type);
		return pTypeExt->IgnoredByMouse.Get();
	}

	return false;
}

DEFINE_HOOK(0x6DA3D2, TacticalClass_GetObjectOnCrd_IgnoredByMouse1, 0x8)
{
	enum { Ignore = 0x6DA491, DontIgnore = 0x6DA3DA };

	GET(ObjectClass* const, pObject, ESI);

	return !pObject || ShouldIgnoreByMouse(pObject) ? Ignore : DontIgnore;
}

DEFINE_HOOK(0x6DA4FB, TacticalClass_GetObjectOnCrd_IgnoredByMouse2, 0x6)
{
	enum { SkipGameCode = 0x6DA501 };

	GET(CellClass* const, pCell, EAX);

	ObjectClass* pFoundObject = nullptr;

	for (auto pOccupier = pCell->FirstObject; pOccupier; pOccupier = pOccupier->NextObject)
	{
		if (ShouldIgnoreByMouse(pOccupier))
			continue;
		pFoundObject = pOccupier;
		break;
	}

	R->EAX(pFoundObject);
	return SkipGameCode;
}

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

#pragma region SharedControl

DEFINE_HOOK(0x50B716, HouseClass_IsCurrentPlayer_SharedControl, 0x6)
{
	GET(HouseClass*, pThis, ECX);
	R->EAX(RulesExt::Global()->AllyShareControl ? pThis->IsAlliedWith(HouseClass::CurrentPlayer) : pThis == HouseClass::CurrentPlayer);
	return 0x50B723;
}

#pragma endregion

#pragma region Activate

DEFINE_HOOK(0x70FC85, TechnoClass_Activate_End, 0x5)
{
	GET(TechnoClass*, pThis, ECX);

	if (!pThis->Deactivated && TechnoExt::Fetch(pThis)->IsWreckage)
		pThis->Deactivate();

	return 0;
}

#pragma region

#pragma region TunnelDist

DEFINE_HOOK(0x74608F, UnitClass_AStarAttempt_SimpleTooFar, 0x5)
{
	enum { GoUnderground = 0x7460F4, GoSurface = 0x746094 };
	GET(const int, simpleDist, EAX);
	return simpleDist >= RulesExt::Global()->TunnelSimpleDistTooFar ? GoUnderground : GoSurface;
}

DEFINE_HOOK(0x7460EC, UnitClass_AStarAttempt_PathingTooFar, 0x5)
{
	GET(const int, pathingDist, EAX);
	R->DL(pathingDist > RulesExt::Global()->TunnelPathingDistTooFar);
	return 0;
}

#pragma region

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

#pragma region LoadGameTips

static inline void LoadTips(INI_EX exINI, const char* pSection, DynamicVectorClass<CSFText>& dest)
{
    char tempBuffer[32];
    for (size_t i = 0; i < 1024; ++i)
    {
        Valueable<CSFText> tip;
        _snprintf_s(tempBuffer, sizeof(tempBuffer), "Tip%d", i);
        tip.Read(exINI, pSection, tempBuffer);

        if (!tip.Get())
            return;

        dest.AddUnique(tip);
    }
}

DEFINE_HOOK(0x68758D, INIClass_ReadScenario_AfterLoadProgressMgrDraw, 0x5)
{
	// Get the text to draw.
	GET(CCINIClass*, pMapINI, EBP);

	// The RulesExt has not been read yet. Thus we manually read the ini here.
	const auto pRulesINI = CCINIClass::INI_Rules;
	auto pText = L"";
	DynamicVectorClass<CSFText> availableTexts;
	const bool useMapTipsOnly = pMapINI->ReadBool(GameStrings::Basic, "UseMapTipsOnly", false);
	INI_EX exMapINI(pMapINI);
	LoadTips(exMapINI, GameStrings::Basic, availableTexts);

	if (!useMapTipsOnly)
	{
		INI_EX exINI(pRulesINI);
		LoadTips(exINI, GameStrings::General, availableTexts);

		if (SessionClass::Instance.GameMode != GameMode::Campaign)
		{
			const int countryIdx = NodeNameType::Array[0]->Country;

			if (countryIdx >= 0 && countryIdx < HouseTypeClass::Array.Count)
			{
				const auto pCountryName = HouseTypeClass::Array.GetItem(countryIdx)->ID;
				LoadTips(exINI, pCountryName, availableTexts);
			}
		}
	}

	if (availableTexts.Count <= 0)
		return 0;

	srand(static_cast<unsigned int>(time(NULL)));
	pText = availableTexts.GetItem(rand() % availableTexts.Count).Text;

	// Calculate the rect.
	constexpr int assumedTextHeight = 15;
	constexpr int gapToBorder = 8;
	constexpr int gapToAnotherLine = 5;
	const int tipBarWidth = pRulesINI->ReadInteger(GameStrings::General, "TipBarWidth", 700);
	const bool tipBarTwoLines = pRulesINI->ReadBool(GameStrings::General, "TipBarTwoLines", false);
	const int width = tipBarWidth;
	const int height = tipBarTwoLines ? (assumedTextHeight * 2 + gapToBorder * 2 + gapToAnotherLine) : (assumedTextHeight + gapToBorder * 2);
	const auto barRect = LoadProgressManager::Instance->LoadBarSHPRect;
	const int x = barRect.X + (barRect.Width - width) / 2;
	const int y = barRect.Y + barRect.Height - height - 5;
	auto rect = RectangleStruct { x, y, width, height };

	// Draw background.
	const auto pSurface = LoadProgressManager::Instance->ProgressSurface;
	pSurface->FillRect(&rect, COLOR_BLACK);
	pSurface->DrawRect(&rect, COLOR_WHITE);

	// Draw the text.
	constexpr TextPrintType printType = TextPrintType::Center | TextPrintType::Point8;

	if (tipBarTwoLines)
	{
		// Split text at first newline.
		// Only support one or two lines.
		const wchar_t* pFirstPart = pText;
		const wchar_t* pSecondPart = nullptr;
		std::unique_ptr<wchar_t[]> pFirstBuffer;

		if (const wchar_t* pNewline = wcschr(pText, L'\n'))
		{
			// Create temporary buffer for first part
			size_t firstLen = pNewline - pText;
			pFirstBuffer = std::make_unique<wchar_t[]>(firstLen + 1);
			wcsncpy(pFirstBuffer.get(), pText, firstLen);
			pFirstBuffer[firstLen] = L'\0';

			pFirstPart = pFirstBuffer.get();
			pSecondPart = pNewline + 1;
		}

		if (pSecondPart)
		{
			auto location = Point2D { rect.Width / 2, gapToBorder };
			pSurface->DrawTextA(pFirstPart, &rect, &location, COLOR_WHITE, 0, printType);
			location = Point2D { rect.Width / 2, gapToBorder + assumedTextHeight + gapToAnotherLine };
			pSurface->DrawTextA(pSecondPart, &rect, &location, COLOR_WHITE, 0, printType);
		}
		else
		{
			const int gapToBorderOneLine = (height - assumedTextHeight) / 2;
			auto location = Point2D { rect.Width / 2, gapToBorderOneLine };
			pSurface->DrawTextA(pText, &rect, &location, COLOR_WHITE, 0, printType);
		}
	}
	else
	{
		auto location = Point2D { rect.Width / 2, gapToBorder };
		pSurface->DrawTextA(pText, &rect, &location, COLOR_WHITE, 0, printType);
	}

	return 0;
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

#pragma region MissileIntercepted

DEFINE_HOOK(0x662FD8, RocketLocomotionClass_Process_CheckHealth, 0x5)
{
	enum { SkipDetonate = 0x662FE6, Detonate = 0x662FDF };
	GET(AircraftClass*, pLinkedTo, ECX);

	if (pLinkedTo->Health <= 0)
	{
		if (AircraftTypeExt::Fetch(pLinkedTo->Type)->Missile_UseDeathWeaponWhenIntercepted)
		{
			pLinkedTo->FireDeathWeapon(0);
			AircraftTrackerClass::Instance.Remove(pLinkedTo);
			pLinkedTo->UnInit();
			return SkipDetonate;
		}
	}
	else if (MapClass::Instance.IsWithinUsableArea(pLinkedTo->GetCoords()))
	{
		return SkipDetonate;
	}

	return Detonate;
}

#pragma endregion

#pragma region NoAutoFire

DEFINE_HOOK(0x6F8E44, TechnoClass_SelectAutoTarget_NoAutoFire, 0x7)
{
	GET(TechnoClass*, pThis, ESI);
	return TechnoTypeExt::Fetch(pThis->GetTechnoType())->NoAutoFire_AI && !pThis->Owner->IsControlledByHuman() ? 0x6F8E38 : 0;
}

#pragma endregion

#pragma region KeepAnimOnLimbo

DEFINE_HOOK(0x422C70, AnimClass_DrawIfVisible_DontDrawIfOwnerInLimbo, 0x6)
{
	if (!RulesExt::Global()->KeepAnimOnLimbo)
		return 0;

	GET(AnimClass*, pThis, ECX);

	R->EAX(pThis->LoopDelay || pThis->OwnerObject && (pThis->OwnerObject->InLimbo || pThis->OwnerObject->VisualCharacter(true, HouseClass::CurrentPlayer) == VisualType::Hidden));
	return R->Origin() + 0x6;
}

DEFINE_HOOK(0x425174, AnimClass_PointerExpired_KeepAnimOnLimbo, 0x6)
{
	if (!RulesExt::Global()->KeepAnimOnLimbo)
		return 0;

	GET_STACK(bool, bRemoved, STACK_OFFSET(0xC, 0x8));

	return bRemoved ? 0 : 0x4251A3;
}

#pragma endregion

// TODO Self-made impl


















































#pragma region NoManualEject

DEFINE_HOOK(0x73D6EC, UnitClass_Unload_NoManualEject, 0x6)
{
	enum { NoEject = 0x73DCD3 };
	GET(TechnoTypeClass* const, pType, EAX);
	return TechnoTypeExt::Fetch(pType)->NoManualEject.Get() ? NoEject : 0;
}

DEFINE_HOOK(0x740015, UnitClass_WhatAction_NoManualEject, 0x6)
{
	enum { NoEject = 0x7400F0 };
	GET(TechnoTypeClass* const, pType, EAX);
	return TechnoTypeExt::Fetch(pType)->NoManualEject.Get() ? NoEject : 0;
}

#pragma endregion

#pragma region InfantrySquad

static inline InfantryClass* CreateInfantryFromFactory(InfantryTypeClass* pType, HouseClass* pOwner)
{
	// BuildLimit check goes before creation
	if (pType->BuildLimit > 0)
	{
		int sum = pOwner->CountOwnedNow(pType);

		// copy Ares' deployable units x build limit fix
		if (auto const pUndeploy = pType->UndeploysInto)
			sum += pOwner->CountOwnedNow(pUndeploy);

		if (sum >= pType->BuildLimit)
			return nullptr;
	}

	return static_cast<InfantryClass*>(pType->CreateObject(pOwner));
}

DEFINE_HOOK(0x444DDF, BuildingClass_KickOutUnit_InfantrySquad, 0x5)
{
	enum { SkipGameCode = 0x444971 };

	GET(BuildingClass*, pThis, ESI);

	const auto pThisType = pThis->Type;

	if (pThisType->Factory == AbstractType::InfantryType || pThisType->Cloning)
	{
		GET(TechnoClass*, pTechno, EDI);

		if (const auto pInfantry = abstract_cast<InfantryClass*, true>(pTechno))
		{
			const auto pExt = TechnoExt::Fetch(pInfantry);
			const auto pTypeExt = pExt->TypeExtData;
			const int size = pTypeExt->Squad_Members.size();

			if (size > 0)
			{
				if (pTypeExt->Squad_IsInitAsTeam)
				{
					pExt->SquadManager = SquadManagerClass::Allocate();
					pExt->SquadManager->Members.reserve(size + 1);
					pExt->SquadManager->AddMember(pInfantry);
				}

				const auto pOwner = pInfantry->Owner;
				const auto pDestination = (pThis->ArchiveTarget ? pThis->ArchiveTarget : pInfantry);

				for (int i = 0; i < size; ++i)
				{
					if (const auto pMember = CreateInfantryFromFactory(pTypeExt->Squad_Members[i], pOwner))
					{
						++Unsorted::ScenarioInit;
						pMember->Unlimbo(pInfantry->Location, DirType::North);
						pMember->SetDestination(pDestination, true);
						pMember->QueueMission(Mission::Move, 0);
						--Unsorted::ScenarioInit;

						if (pExt->SquadManager)
						{
							TechnoExt::Fetch(pMember)->SquadManager = pExt->SquadManager;
							pExt->SquadManager->AddMember(pMember);
						}
					}
				}
			}
		}
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x6FBFD0, TechnoClass_Select_SquadSelect, 0x5)
{
	GET(TechnoClass*, pTechno, ESI);

	if (pTechno->Owner->IsControlledByCurrentPlayer())
	{
		if (const auto pSquadManager = TechnoExt::Fetch(pTechno)->SquadManager)
		{
			if (!pSquadManager->Selecting)
			{
				pSquadManager->Selecting = true;

				for (auto& pMember : pSquadManager->Members)
				{
					if (pMember != pTechno)
						pMember->Select();
				}

				pSquadManager->Selecting = false;
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x737BBE, UnitClass_Unlimbo_CreatePassengerSquad, 0x6)
{
	GET(UnitClass*, pThis, ESI);

	const auto pExt = TechnoExt::Fetch(pThis);

	if (pExt->TypeExtData->Squad_IsInitAsTeam)
	{
		if (pThis->Passengers.NumPassengers > 0)
		{
			pExt->SquadManager = SquadManagerClass::Allocate();
			pExt->SquadManager->Members.reserve(pThis->Passengers.NumPassengers + 1);
			pExt->SquadManager->AddMember(pThis);

			for (auto pPassenger = pThis->Passengers.GetFirstPassenger(); pPassenger; pPassenger = abstract_cast<FootClass*>(pPassenger->NextObject))
			{
				TechnoExt::Fetch(pPassenger)->SquadManager = pExt->SquadManager;
				pExt->SquadManager->AddMember(pPassenger);
			}
		}
	}

	return 0;
}

#pragma endregion
/*
#pragma region SmoothMouseMoving

DEFINE_PATCH(0x7B853C, 0x01);

#pragma endregion
*/
// TODO Other contributors' impl










