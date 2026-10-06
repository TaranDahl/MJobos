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
