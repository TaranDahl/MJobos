#include "Body.h"

#include <SpawnManagerClass.h>
#include <Ext/WeaponType/Body.h>

DEFINE_HOOK(0x6B0B9C, SlaveManagerClass_Killed_DecideOwner, 0x6)
{
	enum { KillTheSlave = 0x6B0BDF, ChangeSlaveOwner = 0x6B0BB4 };

	GET(InfantryClass*, pSlave, ESI);

	if (const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pSlave->Type))
	{
		switch (pTypeExt->Slaved_OwnerWhenMasterKilled.Get())
		{
		case SlaveChangeOwnerType::Suicide:
			return KillTheSlave;

		case SlaveChangeOwnerType::Master:
			R->EAX(pSlave->Owner);
			return ChangeSlaveOwner;

		case SlaveChangeOwnerType::Neutral:
			if (auto pNeutral = HouseClass::FindNeutral())
			{
				R->EAX(pNeutral);
				return ChangeSlaveOwner;
			}

		default: // SlaveChangeOwnerType::Killer
			return 0x0;
		}
	}

	return 0x0;
}

// Fix slaves cannot always suicide due to armor multiplier or something
DEFINE_PATCH(0x6B0BF7,
	0x6A, 0x01  // push 1       // ignoreDefense=false->true
);

DEFINE_HOOK(0x6B7265, SpawnManagerClass_AI_UpdateTimer, 0x6)
{
	GET(SpawnManagerClass* const, pThis, ESI);

	if (pThis->Owner && pThis->Status == SpawnManagerStatus::Launching && pThis->CountDockedSpawns() != 0)
	{
		if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Owner->GetTechnoType()))
		{
			if (pTypeExt->Spawner_DelayFrames.isset())
				R->EAX(std::min(pTypeExt->Spawner_DelayFrames.Get(), 10));
		}
	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x6B73BE, SpawnManagerClass_AI_SpawnTimer, 0x6)
DEFINE_HOOK(0x6B73AD, SpawnManagerClass_AI_SpawnTimer, 0x5)
{
	GET(SpawnManagerClass* const, pThis, ESI);

	if (pThis->Owner)
	{
		if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Owner->GetTechnoType()))
		{
			if (pTypeExt->Spawner_DelayFrames.isset())
				R->ECX(pTypeExt->Spawner_DelayFrames.Get());
		}
	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x6B769F, SpawnManagerClass_AI_InitDestination, 0x7)
DEFINE_HOOK(0x6B7600, SpawnManagerClass_AI_InitDestination, 0x6)
{
	enum { SkipGameCode1 = 0x6B760E, SkipGameCode2 = 0x6B76DE };

	GET(SpawnManagerClass* const, pThis, ESI);
	GET(AircraftClass* const, pSpawnee, EDI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Owner->GetTechnoType());

	if (pTypeExt->Spawner_AttackImmediately)
	{
		pSpawnee->SetTarget(pThis->Target);
		pSpawnee->QueueMission(Mission::Attack, true);
		pSpawnee->IsReturningFromAttackRun = false;
	}
	else
	{
		auto const mapCoords = pThis->Owner->GetMapCoords();
		auto const pCell = MapClass::Instance->GetCellAt(mapCoords);
		pSpawnee->SetDestination(pCell->GetNeighbourCell(FacingType::North), true);
		pSpawnee->QueueMission(Mission::Move, false);
	}

	return R->Origin() == 0x6B7600 ? SkipGameCode1 : SkipGameCode2;
}

DEFINE_HOOK(0x6B77B4, SpawnManagerClass_Update_RecycleSpawned, 0x7)
{
	//enum { RecycleIsOk = 0x6B77FF, RecycleIsNotOk = 0x6B7838 };

	GET(SpawnManagerClass* const, pThis, ESI);
	GET(AircraftClass* const, pSpawned, EDI);
	GET(CellStruct* const, pSpawnerMapCrd, EBP);

	if (!pThis)
		return 0;

	auto pSpawner = pThis->Owner;

	if (!pSpawner || !pSpawned)
		return 0;

	auto const pSpawnerType = pSpawner->GetTechnoType();
	auto spawnedMapCrd = pSpawned->GetMapCoords();

	if (!pSpawnerType)
		return 0;

	auto const pSpawnerExt = TechnoTypeExt::ExtMap.Find(pSpawnerType);

	if (!pSpawnerExt)
		return 0;

	auto spawnerCrd = pSpawner->GetCoords();
	auto spawnedCrd = pSpawned->GetCoords();
	auto recycleCrd = spawnerCrd;
	auto deltaCrd = spawnedCrd - recycleCrd;
	bool shouldRecycleSpawned = false;
	int recycleRange = pSpawnerExt->Spawner_RecycleRange;

	if (recycleRange < 0)
	{
		if (pSpawner->WhatAmI() == AbstractType::Building && deltaCrd.X <= 182 && deltaCrd.Y <= 182 && deltaCrd.Z < 20)
		{
			shouldRecycleSpawned = true;
		}
		if (pSpawner->WhatAmI() != AbstractType::Building && spawnedMapCrd == *pSpawnerMapCrd && deltaCrd.Z < 20)
		{
			shouldRecycleSpawned = true;
		}
	}
	else
	{
		if (deltaCrd.Magnitude() <= recycleRange)
		{
			shouldRecycleSpawned = true;
		}
	}

	if (!shouldRecycleSpawned)
	{
		return 0;
	}
	else
	{
		if (pSpawnerExt->Spawner_RecycleAnim)
		{
			GameCreate<AnimClass>(pSpawnerExt->Spawner_RecycleAnim, spawnedCrd);
		}
		pSpawned->SetLocation(spawnerCrd);
		R->EAX(pSpawnerMapCrd);
		return 0;
	}
}

// I must not regroup my forces.
DEFINE_HOOK(0x739920, UnitClass_TryToDeploy_DisableRegroupAtNewConYard, 0x6)
{
	enum { SkipRegroup = 0x73992B, DoNotSkipRegroup = 0 };

	auto const pRules = RulesExt::Global();
	if (!pRules->GatherWhenMCVDeploy)
		return SkipRegroup;
	else
		return DoNotSkipRegroup;
}

DEFINE_HOOK(0x6F7891, TechnoClass_IsCloseEnough_CylinderRangefinding, 0x7)
{
	enum { ret = 0x6F789A };

	GET(WeaponTypeClass* const, pWeaponType, EDI);
	GET(TechnoClass* const, pThis, ESI);

	auto const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeaponType);
	bool cylinder = RulesExt::Global()->CylinderRangefinding;

	if (pWeaponExt)
	{
		cylinder = pWeaponExt->CylinderRangefinding.Get(cylinder);
	}

	if (cylinder)
	{
		R->AL(true);
	}
	else
	{
		R->AL(pThis->IsInAir()); // vanilla check
	}

	return ret;
}

bool __fastcall BuildingTypeClass_CanUseWaypoint(BuildingTypeClass* pThis)
{
	return RulesExt::Global()->BuildingWaypoint;
}

bool __fastcall AircraftTypeClass_CanUseWaypoint(AircraftTypeClass* pThis)
{
	return RulesExt::Global()->AircraftWaypoint;
}

DEFINE_JUMP(VTABLE, 0x7E4610, GET_OFFSET(BuildingTypeClass_CanUseWaypoint))
DEFINE_JUMP(VTABLE, 0x7E2908, GET_OFFSET(AircraftTypeClass_CanUseWaypoint))

DEFINE_HOOK(0x6FA697, TechnoClass_Update_DontScanIfUnarmed, 0x6)
{
	enum { SkipTargeting = 0x6FA6F5, DoTargeting = 0 };

	GET(TechnoClass*, pThis, ESI);

	if (pThis->IsArmed())
		return DoTargeting;
	else
		return SkipTargeting;
}

DEFINE_HOOK(0x709866, TechnoClass_TargetAndEstimateDamage_ScanDelayGuardArea, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	auto const pOwner = pThis->Owner;
	auto const pRulesExt = RulesExt::Global();
	auto const pRules = RulesClass::Instance();
	int delay = 1;

	if (pOwner->IsHumanPlayer || pOwner->IsControlledByHuman())
	{
		delay = pTypeExt->PlayerGuardAreaTargetingDelay.Get(pRulesExt->PlayerGuardAreaTargetingDelay.Get(pRules->GuardAreaTargetingDelay));
	}
	else
	{
		delay = pTypeExt->AIGuardAreaTargetingDelay.Get(pRulesExt->AIGuardAreaTargetingDelay.Get(pRules->GuardAreaTargetingDelay));
	}

	R->ECX(delay);

	return 0;
}

DEFINE_HOOK(0x70989C, TechnoClass_TargetAndEstimateDamage_ScanDelayNormal, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	auto const pOwner = pThis->Owner;
	auto const pRulesExt = RulesExt::Global();
	auto const pRules = RulesClass::Instance();
	int delay = (pThis->Location.X + pThis->Location.Y + Unsorted::CurrentFrame) % 3;

	if (pOwner->IsHumanPlayer || pOwner->IsControlledByHuman())
	{
		delay += pTypeExt->PlayerNormalTargetingDelay.Get(pRulesExt->PlayerNormalTargetingDelay.Get(pRules->NormalTargetingDelay));
	}
	else
	{
		delay += pTypeExt->AINormalTargetingDelay.Get(pRulesExt->AINormalTargetingDelay.Get(pRules->NormalTargetingDelay));
	}

	R->ECX(delay);

	return 0;
}

//DEFINE_HOOK(0x63A4D4, UnknownClass_PlanWaypoint_ContinuePlanningWaypoint1, 0x7)
//{
//	return 0x63A573;
//}

DEFINE_HOOK(0x63745D, UnknownClass_PlanWaypoint_ContinuePlanningOnEnter1, 0x6)
{
	enum { SkipDeselect = 0x637468, DoNotSkip = 0 };

	GET(int, planResult, ESI);

	if (planResult == 0 && !RulesExt::Global()->StopPlanningOnEnter)
		return SkipDeselect;
	else
		return DoNotSkip;
}

DEFINE_HOOK(0x637479, UnknownClass_PlanWaypoint_DisableMessage1, 0x5)
{
	enum { SkipMessage = 0x63748A, DoNotSkip = 0 };

	if (!RulesExt::Global()->StopPlanningOnEnter)
		return SkipMessage;
	else
		return DoNotSkip;
}

DEFINE_HOOK(0x637491, UnknownClass_PlanWaypoint_DisableMessage2, 0x5)
{
	enum { SkipMessage = 0x637524, DoNotSkip = 0 };

	if (!RulesExt::Global()->StopPlanningOnEnter)
		return SkipMessage;
	else
		return DoNotSkip;
}

DEFINE_HOOK(0x638D73, UnknownClass_CheckLastWaypoint_ContinuePlanningWaypoint2, 0x5)
{
	enum { SkipDeselect = 0x638D8D, DoNotSkip = 0 };

	if (!RulesExt::Global()->StopPlanningOnEnter)
		return SkipDeselect;
	else
		return DoNotSkip;
}
