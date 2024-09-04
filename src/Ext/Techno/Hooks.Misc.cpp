#include "Body.h"
#include "Ext/BulletType/Body.h"

#include <Ext/WeaponType/Body.h>
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

DEFINE_HOOK(0x41847E, AircraftClass_MissionAttack_ScatterCell1, 0x6)
{
	enum { SkipScatter = 0x4184C2, Scatter = 0 };
	return RulesExt::Global()->StrafingTargetScatter ? Scatter : SkipScatter;
}

DEFINE_HOOK(0x4186DD, AircraftClass_MissionAttack_ScatterCell2, 0x5)
{
	enum { SkipScatter = 0x418720, Scatter = 0 };
	return RulesExt::Global()->StrafingTargetScatter ? Scatter : SkipScatter;
}

DEFINE_HOOK(0x41882C, AircraftClass_MissionAttack_ScatterCell3, 0x6)
{
	enum { SkipScatter = 0x418870, Scatter = 0 };
	return RulesExt::Global()->StrafingTargetScatter ? Scatter : SkipScatter;
}

DEFINE_HOOK(0x41893B, AircraftClass_MissionAttack_ScatterCell4, 0x6)
{
	enum { SkipScatter = 0x41897F, Scatter = 0 };
	return RulesExt::Global()->StrafingTargetScatter ? Scatter : SkipScatter;
}

DEFINE_HOOK(0x418A4A, AircraftClass_MissionAttack_ScatterCell5, 0x6)
{
	enum { SkipScatter = 0x418A8E, Scatter = 0 };
	return RulesExt::Global()->StrafingTargetScatter ? Scatter : SkipScatter;
}

DEFINE_HOOK(0x418B46, AircraftClass_MissionAttack_ScatterCell6, 0x6)
{
	enum { SkipScatter = 0x418B8A, Scatter = 0 };
	return RulesExt::Global()->StrafingTargetScatter ? Scatter : SkipScatter;
}

// 航味麻酱: These are WW's bullshit checks.
//
//if (  bHasAElite
//   || ignoreDestination
//   || RulesClass::Instance->PlayerScatter
//   || pTechnoToScatter && (FootClass::HasAbility(pTechnoToScatter, Ability::Scatter)
//   || pTechnoToScatter->Owner->IQLevel2 >= RulesClass::Instance->Scatter) )

// delete the first one 'bHasAElite' and the second one 'ignoreDestination'
// fix the third one 'RulesClass::Instance->PlayerScatter'
DEFINE_HOOK(0x481778, CellClass_ScatterContent_Fix, 0x6)
{
	enum { ret = 0x481793 };
	GET(ObjectClass*, pObject, ESI);

	auto pTechno = abstract_cast<TechnoClass*>(pObject);

	if (RulesClass::Instance()->PlayerScatter && pTechno && pTechno->Owner->IsHumanPlayer)
		R->CL(true);

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

DEFINE_HOOK(0x63745D, UnknownClass_PlanWaypoint_ContinuePlanningOnEnter, 0x6)
{
	enum { SkipDeselect = 0x637468 };

	GET(int, planResult, ESI);

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

	GET(Action, action, EAX);

	if (!RulesExt::Global()->StopPlanningOnEnter)
		return SkipDeselect;
	else if (action == Action::Select || action == Action::ToggleSelect || action == Action::Capture)
		return Deselect;

	return SkipDeselect;
}

// 航味麻酱: No idea about why these did not works. Not important though. Here is the assembly:
// 00418B4A 0E0                 push    1               ; ignoreMission // I want to change this '1' to zero conditionally to make it not always ignore mission.
// 00418B4C 0E4                 mov     eax, [edx]                      // I don't know if my code is working as my purpose. I don't quite understand assembly.
//                                                                      // If it is, then the forced scatter is not just a problem with this boolean.
//
//DEFINE_HOOK_AGAIN(0x418484, AircraftClass_MissionAttack_ScatterIgnoreMission, 0x6)
//DEFINE_HOOK_AGAIN(0x4186E2, AircraftClass_MissionAttack_ScatterIgnoreMission, 0xA)
//DEFINE_HOOK_AGAIN(0x418832, AircraftClass_MissionAttack_ScatterIgnoreMission, 0xC)
//DEFINE_HOOK_AGAIN(0x418941, AircraftClass_MissionAttack_ScatterIgnoreMission, 0x6)
//DEFINE_HOOK_AGAIN(0x418A50, AircraftClass_MissionAttack_ScatterIgnoreMission, 0x6)
//DEFINE_HOOK(0x418B4C, AircraftClass_MissionAttack_ScatterIgnoreMission, 0xA)
//{
//	R->ESP(false);
//	return 0;
//}
DEFINE_HOOK(0x702B31, TechnoClass_ReceiveDamage_ReturnFireCheck, 0x7)
{
	enum { SkipReturnFire = 0x702B47, NotSkip = 0 };

	GET(TechnoClass*, pThis, ESI);

	auto pOwner = pThis->Owner;

	if (!(pOwner->IsHumanPlayer || pOwner->IsInPlayerControl) || !RulesExt::Global()->PlayerReturnFire_Smarter)
		return NotSkip;

	auto pTarget = pThis->Target;

	if (pTarget)
		return SkipReturnFire;

	auto mission = pThis->CurrentMission;
	auto pThisFoot = abstract_cast<FootClass*>(pThis);
	bool isJJMoving = pThisFoot != 0 ? pThisFoot->GetHeight() > Unsorted::CellHeight && mission == Mission::Move && pThisFoot->Locomotor->Is_Moving_Now() : 0; // I have really no idea about how to check this perfectly.
	bool isMoving = pThisFoot != 0 ? isJJMoving || (mission == Mission::Move && pThisFoot->GetHeight() <= 0) : 0;

	if (isMoving)
		return SkipReturnFire;
	else
		return NotSkip;
}

DEFINE_HOOK(0x6FC22A, TechnoClass_GetFireError_TargetingIronCurtain, 0x6)
{
	enum { CantFire = 0x6FC86A, GoOtherChecks = 0x6FC24D };

	GET(TechnoClass*, pThis, ESI);
	GET(ObjectClass*, pTarget, EBP);
	GET_STACK(int, wpIdx, STACK_OFFSET(0x20, 0x8));

	auto const pRules = RulesExt::Global();
	auto pOwner = pThis->Owner;

	if (!pTarget->IsIronCurtained())
		return GoOtherChecks;

	auto pWpExt = WeaponTypeExt::ExtMap.Find(pThis->GetWeapon(wpIdx)->WeaponType);
	bool isPlayer = pOwner->IsHumanPlayer || pOwner->IsInPlayerControl;
	bool isHealing = pThis->CombatDamage(wpIdx) < 0;

	if (isPlayer ? pRules->PlayerAttackIronCurtain : pRules->AIAttackIronCurtain)
		return GoOtherChecks;
	else if (pWpExt && pWpExt->AttackIronCurtain.Get(isHealing))
		return GoOtherChecks;
	else
		return CantFire;
}

DEFINE_HOOK(0x6F50A9, TechnoClass_UpdatePosition_TemporalLetGo, 0x7)
{
	enum { LetGo = 0x6F50B4, SkipLetGo = 0x6F50B9 };

	GET(TechnoClass* const, pThis, ESI);
	GET(TemporalClass* const, pTemporal, ECX);

	if (!pTemporal)
		return SkipLetGo;

	if (!pTemporal->Target)
		return SkipLetGo;

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	if (!pTypeExt || !pTypeExt->KeepWarping)
		return LetGo;
	else
		return SkipLetGo;
}

DEFINE_HOOK(0x709A43, TechnoClass_EnterIdleMode_TemporalLetGo, 0x7)
{
	enum { LetGo = 0x709A54, SkipLetGo = 0x709A59 };

	GET(TechnoClass* const, pThis, ESI);
	GET(TemporalClass* const, pTemporal, ECX);

	if (!pTemporal)
		return SkipLetGo;

	if (!pTemporal->Target)
		return SkipLetGo;

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	if (!pTypeExt || !pTypeExt->KeepWarping)
		return LetGo;
	else
		return SkipLetGo;
}

DEFINE_HOOK(0x70023B, TechnoClass_MouseOverObject_AttackUnderGround, 0x5)
{
	enum { FireIsOK = 0x700246, FireIsNotOK = 0x70056C };

	GET(ObjectClass*, pObject, EDI);
	GET(TechnoClass*, pThis, ESI);
	GET(int, wpIdx, EAX);

	auto const pWeapon = pThis->GetWeapon(wpIdx)->WeaponType;
	auto const pProjExt = pWeapon ? BulletTypeExt::ExtMap.Find(pWeapon->Projectile) : 0;
	bool isSurfaced = pObject->IsSurfaced();

	if (!isSurfaced && (!pProjExt || !pProjExt->AU))
		return FireIsNotOK;

	return FireIsOK;
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

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pTechno->GetTechnoType());

	if (pTechno->InOpenToppedTransport || (pTypeExt && pTypeExt->KeepWarping))
		return CheckRange;

	return DontCheckRange;
}
