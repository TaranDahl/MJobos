#include "Body.h"

#include <EventClass.h>
#include <SpawnManagerClass.h>

#include <Ext/Building/Body.h>
#include <Ext/WeaponType/Body.h>
#include "Ext/BulletType/Body.h"

#pragma region NoBurstDelay

DEFINE_HOOK(0x5209EE, InfantryClass_UpdateFiring_BurstNoDelay, 0x5)
{
	enum { SkipVanillaFire = 0x520A57 };

	GET(InfantryClass* const, pThis, EBP);
	GET(const int, wpIdx, ESI);
	GET(AbstractClass* const, pTarget, EAX);

	if (const WeaponTypeClass* const pWeapon = pThis->GetWeapon(wpIdx)->WeaponType)
	{
		if (pWeapon->Burst > 1)
		{
			if (const WeaponTypeExt::ExtData* const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon))
			{
				if (pWeaponExt->Burst_NoDelay)
				{
					if (pThis->Fire(pTarget, wpIdx))
					{
						int rof = pThis->RearmTimer.TimeLeft;
						pThis->RearmTimer.Start(0);

						for (int i = 1; i != pWeapon->Burst && pThis->GetFireError(pTarget, wpIdx, true) == FireError::OK && pThis->Fire(pTarget, wpIdx); ++i)
						{
							rof = pThis->RearmTimer.TimeLeft;
							pThis->RearmTimer.Start(0);
						}

						pThis->RearmTimer.Start(rof);
					}

					return SkipVanillaFire;
				}
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x736F67, UnitClass_UpdateFiring_BurstNoDelay, 0x6)
{
	enum { SkipVanillaFire = 0x737063 };

	GET(UnitClass* const, pThis, ESI);
	GET(const int, wpIdx, EDI);
	GET(AbstractClass* const, pTarget, EAX);

	if (const WeaponTypeClass* const pWeapon = pThis->GetWeapon(wpIdx)->WeaponType)
	{
		if (pWeapon->Burst > 1)
		{
			if (const WeaponTypeExt::ExtData* const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon))
			{
				if (pWeaponExt->Burst_NoDelay)
				{
					if (pThis->Fire(pTarget, wpIdx))
					{
						int rof = pThis->RearmTimer.TimeLeft;
						pThis->RearmTimer.Start(0);

						for (int i = 1; i != pWeapon->Burst && pThis->GetFireError(pTarget, wpIdx, true) == FireError::OK && pThis->Fire(pTarget, wpIdx); ++i)
						{
							rof = pThis->RearmTimer.TimeLeft;
							pThis->RearmTimer.Start(0);
						}

						pThis->RearmTimer.Start(rof);
					}

					return SkipVanillaFire;
				}
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x44B630, BuildingClass_MissionAttack_AnimDelayedFire, 0x6)
{
	enum { JustFire = 0x44B6C4 };

	GET(BuildingClass* const, pThis, ESI);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->AnimDontDelayBurst && pThis->CurrentBurstIndex != 0) ? JustFire : 0;
}

#pragma endregion

#pragma region AIConstructionYard

DEFINE_HOOK(0x740A11, UnitClass_Mission_Guard_AIAutoDeployMCV, 0x6)
{
	enum { SkipGameCode = 0x740A50 };

	GET(UnitClass*, pMCV, ESI);

	return (RulesExt::Global()->AIAutoDeployMCV && pMCV->Owner->NumConYards > 0) ? SkipGameCode : 0;
}

DEFINE_HOOK(0x739889, UnitClass_TryToDeploy_AISetBaseCenter, 0x6)
{
	enum { SkipGameCode = 0x73992B };

	GET(UnitClass*, pMCV, EBP);

	return (RulesExt::Global()->AISetBaseCenter && pMCV->Owner->NumConYards > 1) ? SkipGameCode : 0;
}

DEFINE_HOOK(0x4FD538, HouseClass_AIHouseUpdate_CheckAIBaseCenter, 0x7)
{
	if (RulesExt::Global()->AIBiasSpawnCell && SessionClass::Instance->GameMode != GameMode::Campaign)
	{
		GET(HouseClass*, pAI, EBX);

		if (const int count = pAI->ConYards.Count)
		{
			const int wayPoint = pAI->GetSpawnPosition();

			if (wayPoint != -1)
			{
				const CellStruct center = ScenarioClass::Instance->GetWaypointCoords(wayPoint);
				CellStruct newCenter = center;
				double distanceSquared = 131072.0;

				for (int i = 0; i < count; ++i)
				{
					if (BuildingClass* const pBuilding = pAI->ConYards.GetItem(i))
					{
						if (pBuilding->IsAlive && pBuilding->Health && !pBuilding->InLimbo)
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
					pAI->Base.BaseNodes.Items->MapCoords = newCenter;
					pAI->Base.Center = newCenter;
				}
			}
		}
	}

	return 0;
}
/*
DEFINE_HOOK(0x4FE42F, HouseClass_AIBaseConstructionUpdate_SkipConYards, 0x6)
{
	enum { SkipGameCode = 0x4FE443 };

	GET(BuildingTypeClass*, pType, EAX);

	return (RulesExt::Global()->AIForbidConYard && pType->ConstructionYard) ? SkipGameCode : 0;
}

DEFINE_HOOK(0x505550, HouseClass_AIBaseConstructionUpdate_SkipConYards, 0x6)
{
	enum { SkipGameCode = 0x5056C1 };

	GET(BuildingTypeClass*, pType, ESI);

	return (RulesExt::Global()->AIForbidConYard && pType->ConstructionYard) ? SkipGameCode : 0;
}
*/
#pragma endregion

#pragma region KickOutStuckUnits

// Kick out stuck units when the factory building is not busy
DEFINE_HOOK(0x450248, BuildingClass_UpdateFactory_KickOutStuckUnits, 0x6)
{
	GET(BuildingClass*, pThis, ESI);

	if (!(Unsorted::CurrentFrame % 15))
	{
		BuildingTypeClass* const pType = pThis->Type;

		if (pType->Factory == AbstractType::UnitType && pType->WeaponsFactory && !pType->Naval && pThis->QueuedMission != Mission::Unload)
		{
			const Mission mission = pThis->CurrentMission;

			if (mission == Mission::Guard || (mission == Mission::Unload && pThis->MissionStatus == 1))
				BuildingExt::KickOutStuckUnits(pThis);
		}
	}

	return 0;
}

// Should not kick out units if the factory building is in construction process
DEFINE_HOOK(0x4444A0, BuildingClass_KickOutUnit_NoKickOutInConstruction, 0xA)
{
	enum { ThisIsOK = 0x444565, ThisIsNotOK = 0x4444B3};

	GET(BuildingClass* const, pThis, ESI);

	const Mission mission = pThis->GetCurrentMission();

	return (mission == Mission::Unload || mission == Mission::Construction) ? ThisIsNotOK : ThisIsOK;
}

#pragma endregion

#pragma region NewWaypoints

bool __fastcall BuildingTypeClass_CanUseWaypoint(BuildingTypeClass* pThis)
{
	return RulesExt::Global()->BuildingWaypoint;
}
DEFINE_JUMP(VTABLE, 0x7E4610, GET_OFFSET(BuildingTypeClass_CanUseWaypoint))

bool __fastcall AircraftTypeClass_CanUseWaypoint(AircraftTypeClass* pThis)
{
	return RulesExt::Global()->AircraftWaypoint;
}
DEFINE_JUMP(VTABLE, 0x7E2908, GET_OFFSET(AircraftTypeClass_CanUseWaypoint))

#pragma endregion

#pragma region EngineerAutoFire

DEFINE_HOOK(0x707E84, TechnoClass_GetGuardRange_Engineer, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return 0;
}

DEFINE_HOOK(0x6F8EF1, TechnoClass_SelectAutoTarget_Engineer, 0x6)
{
	enum { SkipGameCode = 0x6F8EF7 };

	GET(InfantryTypeClass* const, pType, EAX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	R->CL(pType->Engineer && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

DEFINE_HOOK(0x709249, TechnoClass_CanPassiveAcquireNow_Engineer1, 0xA)
{
	enum { SkipGameCode = 0x709253 };

	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

DEFINE_HOOK(0x6F8AEC, TechnoClass_TryAutoTargetObject_Engineer1, 0x6)
{
	enum { SkipGameCode = 0x6F8AF2 };

	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

DEFINE_HOOK(0x6F8BB2, TechnoClass_TryAutoTargetObject_Engineer2, 0x6)
{
	enum { SkipGameCode = 0x6F8BB8 };

	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

#pragma endregion

#pragma region AttackWall

DEFINE_HOOK(0x6F8C18, TechnoClass_ScanToAttackWall_PlayerDestroyWall, 0x6)
{
	enum { SkipIsAIChecks = 0x6F8C52 };
	return RulesExt::Global()->PlayerDestroyWalls ? SkipIsAIChecks : 0;
}

DEFINE_HOOK(0x6F8D32, TechnoClass_ScanToAttackWall_DestroyOwnerlessWalls, 0x9)
{
	enum { GoOtherChecks = 0x6F8D58, NotOkToFire = 0x6F8DE3 };

	GET(int, OwnerIdx, EAX);
	GET(TechnoClass*, pThis, ESI);

	if (auto const pOwner = (OwnerIdx != -1) ? HouseClass::Array->Items[OwnerIdx] : nullptr)
	{
		if (pOwner->IsAlliedWith(pThis->Owner)
			&& (!RulesExt::Global()->DestroyOwnerlessWalls
			|| (pOwner != HouseClass::FindSpecial()
			&& pOwner != HouseClass::FindCivilianSide()
			&& pOwner != HouseClass::FindNeutral())))
		{
			return NotOkToFire;
		}
	}

	return GoOtherChecks;
}

#pragma endregion

#pragma region CylinderRange

DEFINE_HOOK(0x6F7891, TechnoClass_IsCloseEnough_CylinderRangefinding, 0x7)
{
	enum { SkipGameCode = 0x6F789A };

	GET(WeaponTypeClass* const, pWeaponType, EDI);
	GET(TechnoClass* const, pThis, ESI);

	bool cylinder = RulesExt::Global()->CylinderRangefinding;

	if (auto const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeaponType))
		cylinder = pWeaponExt->CylinderRangefinding.Get(cylinder);

	R->AL(cylinder ? true : pThis->IsInAir());
	return SkipGameCode;
}

#pragma endregion

#pragma region RecycleSpawned

DEFINE_HOOK(0x6B77B4, SpawnManagerClass_Update_RecycleSpawned, 0x7)
{
	//enum { RecycleIsOk = 0x6B77FF, RecycleIsNotOk = 0x6B7838 };

	GET(SpawnManagerClass* const, pThis, ESI);
	GET(AircraftClass* const, pSpawned, EDI);
	GET(CellStruct* const, pSpawnerMapCrd, EBP);

	if (!pThis)
		return 0;

	auto const pSpawner = pThis->Owner;

	if (!pSpawner || !pSpawned)
		return 0;

	auto const pSpawnerType = pSpawner->GetTechnoType();
	auto const spawnedMapCrd = pSpawned->GetMapCoords();

	if (!pSpawnerType)
		return 0;

	auto const pSpawnerExt = TechnoTypeExt::ExtMap.Find(pSpawnerType);

	if (!pSpawnerExt)
		return 0;

	auto const spawnerCrd = pSpawner->GetCoords();
	auto const spawnedCrd = pSpawned->GetCoords();
	auto const deltaCrd = spawnedCrd - spawnerCrd;
	const int recycleRange = pSpawnerExt->Spawner_RecycleRange;

	if (recycleRange < 0)
	{
		if (pSpawner->WhatAmI() == AbstractType::Building)
		{
			if (deltaCrd.X > 182 || deltaCrd.Y > 182 || deltaCrd.Z >= 20)
				return 0;
		}
		else if (spawnedMapCrd != *pSpawnerMapCrd || deltaCrd.Z >= 20)
		{
			return 0;
		}
	}
	else if (deltaCrd.Magnitude() > recycleRange)
	{
		return 0;
	}

	if (pSpawnerExt->Spawner_RecycleAnim)
		GameCreate<AnimClass>(pSpawnerExt->Spawner_RecycleAnim, spawnedCrd);

	pSpawned->SetLocation(spawnerCrd);
	R->EAX(pSpawnerMapCrd);
	return 0;
}

#pragma endregion

#pragma region ScatterFix

DEFINE_HOOK(0x481778, CellClass_ScatterContent_Fix, 0x6)
{
	enum { SkipGameCode = 0x481793 };
	GET(ObjectClass* const, pObject, ESI);

	auto const pTechno = abstract_cast<TechnoClass*>(pObject);

	R->CL(pTechno && pTechno->Owner->IsHumanPlayer && RulesClass::Instance()->PlayerScatter);
	return SkipGameCode;
}

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

#pragma region ScanDelay

DEFINE_HOOK(0x6FA697, TechnoClass_Update_DontScanIfUnarmed, 0x6)
{
	enum { SkipTargeting = 0x6FA6F5 };

	GET(TechnoClass* const, pThis, ESI);

	return pThis->IsArmed() ? 0 : SkipTargeting;
}

DEFINE_HOOK(0x709866, TechnoClass_TargetAndEstimateDamage_ScanDelayGuardArea, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	auto const pOwner = pThis->Owner;
	auto const pRulesExt = RulesExt::Global();
	auto const pRules = RulesClass::Instance();
	int delay = 1;

	if (pOwner->IsHumanPlayer || pOwner->IsControlledByHuman())
		delay = pTypeExt->PlayerGuardAreaTargetingDelay.Get(pRulesExt->PlayerGuardAreaTargetingDelay.Get(pRules->GuardAreaTargetingDelay));
	else
		delay = pTypeExt->AIGuardAreaTargetingDelay.Get(pRulesExt->AIGuardAreaTargetingDelay.Get(pRules->GuardAreaTargetingDelay));

	R->ECX(delay);
	return 0;
}

DEFINE_HOOK(0x70989C, TechnoClass_TargetAndEstimateDamage_ScanDelayNormal, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	auto const pOwner = pThis->Owner;
	auto const pRulesExt = RulesExt::Global();
	auto const pRules = RulesClass::Instance();
	int delay = (pThis->Location.X + pThis->Location.Y + Unsorted::CurrentFrame) % 3;

	if (pOwner->IsHumanPlayer || pOwner->IsControlledByHuman())
		delay += pTypeExt->PlayerNormalTargetingDelay.Get(pRulesExt->PlayerNormalTargetingDelay.Get(pRules->NormalTargetingDelay));
	else
		delay += pTypeExt->AINormalTargetingDelay.Get(pRulesExt->AINormalTargetingDelay.Get(pRules->NormalTargetingDelay));

	R->ECX(delay);
	return 0;
}

#pragma endregion

#pragma region TargetIronCurtain

DEFINE_HOOK(0x6FC22A, TechnoClass_GetFireError_TargetingIronCurtain, 0x6)
{
	enum { CantFire = 0x6FC86A, GoOtherChecks = 0x6FC24D };

	GET(TechnoClass*, pThis, ESI);
	GET(ObjectClass*, pTarget, EBP);
	GET_STACK(int, wpIdx, STACK_OFFSET(0x20, 0x8));

	if (!pTarget->IsIronCurtained())
		return GoOtherChecks;

	auto pOwner = pThis->Owner;
	auto const pRules = RulesExt::Global();

	if ((pOwner->IsHumanPlayer || pOwner->IsInPlayerControl) ? pRules->PlayerAttackIronCurtain : pRules->AIAttackIronCurtain)
		return GoOtherChecks;

	auto pWpExt = WeaponTypeExt::ExtMap.Find(pThis->GetWeapon(wpIdx)->WeaponType);
	bool isHealing = pThis->CombatDamage(wpIdx) < 0;

	return (pWpExt && pWpExt->AttackIronCurtain.Get(isHealing)) ? GoOtherChecks : CantFire;
}

#pragma endregion

#pragma region KeepTemporal

DEFINE_HOOK(0x6F50A9, TechnoClass_UpdatePosition_TemporalLetGo, 0x7)
{
	enum { LetGo = 0x6F50B4, SkipLetGo = 0x6F50B9 };

	GET(TechnoClass* const, pThis, ESI);
	GET(TemporalClass* const, pTemporal, ECX);

	if (!pTemporal || !pTemporal->Target)
		return SkipLetGo;

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	return (!pTypeExt || !pTypeExt->KeepWarping) ? LetGo : SkipLetGo;
}

DEFINE_HOOK(0x709A43, TechnoClass_EnterIdleMode_TemporalLetGo, 0x7)
{
	enum { LetGo = 0x709A54, SkipLetGo = 0x709A59 };

	GET(TechnoClass* const, pThis, ESI);
	GET(TemporalClass* const, pTemporal, ECX);

	if (!pTemporal || !pTemporal->Target)
		return SkipLetGo;

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	return (!pTypeExt || !pTypeExt->KeepWarping) ? LetGo : SkipLetGo;
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

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pTechno->GetTechnoType());

	return (pTypeExt && pTypeExt->KeepWarping) ? CheckRange : DontCheckRange;
}

#pragma endregion

#pragma region AttackUnderGround

DEFINE_HOOK(0x70023B, TechnoClass_MouseOverObject_AttackUnderGround, 0x5)
{
	enum { FireIsOK = 0x700246, FireIsNotOK = 0x70056C };

	GET(ObjectClass*, pObject, EDI);
	GET(TechnoClass*, pThis, ESI);
	GET(int, wpIdx, EAX);

	if (pObject->IsSurfaced())
		return FireIsOK;

	auto const pWeapon = pThis->GetWeapon(wpIdx)->WeaponType;
	auto const pProjExt = pWeapon ? BulletTypeExt::ExtMap.Find(pWeapon->Projectile) : nullptr;

	return (!pProjExt || !pProjExt->AU) ? FireIsNotOK : FireIsOK;
}

#pragma endregion

#pragma region GuardRange

DEFINE_HOOK(0x4D6E83, FootClass_MissionAreaGuard_FollowStray, 0x6)
{
	enum { SkipGameCode = 0x4D6E8F };

	GET(FootClass* const, pThis, ESI);

	int range = RulesClass::Instance()->GuardModeStray;

	if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType()))
		range = pThis->Owner->IsControlledByHuman() ? pTypeExt->PlayerGuardModeStray.Get(Leptons(range)) : pTypeExt->AIGuardModeStray.Get(Leptons(range));

	R->EDI(range);
	return SkipGameCode;
}

DEFINE_HOOK(0x4D6E97, FootClass_MissionAreaGuard_Pursuit, 0x6)
{
	enum { KeepTarget = 0x4D6ED1, RemoveTarget = 0x4D6EB3 };

	GET(FootClass* const, pThis, ESI);
	GET(int, range, EDI);
	GET(AbstractClass* const, pFocus, EAX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	bool isPlayer = pThis->Owner->IsControlledByHuman();
	bool pursuit = true;

	if (pTypeExt)
		pursuit = isPlayer ? pTypeExt->PlayerGuardModePursuit.Get(RulesExt::Global()->PlayerGuardModePursuit) : pTypeExt->AIGuardModePursuit.Get(RulesExt::Global()->AIGuardModePursuit);

	if ((pFocus->AbstractFlags & AbstractFlags::Foot) == AbstractFlags::None && pTypeExt)
	{
		Leptons stationaryStray = isPlayer ? pTypeExt->PlayerGuardStationaryStray.Get(RulesExt::Global()->PlayerGuardStationaryStray) : pTypeExt->AIGuardStationaryStray.Get(RulesExt::Global()->AIGuardStationaryStray);

		if (stationaryStray != Leptons(-256))
			range = stationaryStray;
	}

	if (pursuit)
	{
		if (!pThis->IsFiring && !pThis->Destination && pThis->DistanceFrom(pFocus) > range)
			return RemoveTarget;
	}
	else if (pThis->DistanceFrom(pFocus) > range)
	{
		return RemoveTarget;
	}

	return KeepTarget;
}

DEFINE_HOOK(0x707F08, TechnoClass_GetGuardRange_AreaGuardRange, 0x5)
{
	enum { SkipGameCode = 0x707E70 };

	GET(Leptons, guardRange, EAX);
	GET(int, mode, EDI);
	GET(TechnoClass* const, pThis, ESI);

	const bool isPlayer = pThis->Owner->IsControlledByHuman();
	auto const pRulesExt = RulesExt::Global();

	double multiplier = pRulesExt->PlayerGuardModeGuardRangeMultiplier;
	Leptons addend = pRulesExt->PlayerGuardModeGuardRangeAddend;

	if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType()))
	{
		multiplier = isPlayer ? pTypeExt->PlayerGuardModeGuardRangeMultiplier.Get(pRulesExt->PlayerGuardModeGuardRangeMultiplier) : pTypeExt->AIGuardModeGuardRangeMultiplier.Get(pRulesExt->AIGuardModeGuardRangeMultiplier);
		addend = isPlayer ? pTypeExt->PlayerGuardModeGuardRangeAddend.Get(pRulesExt->PlayerGuardModeGuardRangeAddend) : pTypeExt->AIGuardModeGuardRangeAddend.Get(pRulesExt->AIGuardModeGuardRangeAddend);
	}

	const Leptons areaGuardRange = Leptons(static_cast<int>(static_cast<int>(guardRange) * multiplier + static_cast<int>(addend)));
	const Leptons min = Leptons((mode == 2) ? 1792 : 0);
	const Leptons max = isPlayer ? pRulesExt->PlayerGuardModeGuardRangeMax : pRulesExt->AIGuardModeGuardRangeMax;

	R->EAX(Math::clamp(areaGuardRange, min, max));

	return SkipGameCode;
}

DEFINE_HOOK(0x4D6D34, FootClass_MissionAreaGuard_Miner, 0x5)
{
	enum { GuardArea = 0x4D6D69 };

	GET(FootClass*, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	return (pTypeExt && pTypeExt->Harvester_CanGuardArea) ? GuardArea : 0;
}

#pragma endregion

#pragma region Bunkerable

DEFINE_HOOK(0x70FB73, FootClass_IsBunkerableNow_Dehardcode, 0x6)
{
	enum { SkipVanillaChecks = 0x70FBAF };

	GET(TechnoTypeClass*, pType, EAX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	return (pTypeExt && pTypeExt->BunkerableAnyWay) ? SkipVanillaChecks : 0;
}

#pragma endregion

#pragma region MissileSpawnFLH

DEFINE_HOOK(0x6B73EA, SpawnManagerClass_Update_MissileSpawnFLH, 0x5)
{
	enum { SkipCurrentBurstReset = 0x6B73FC };

	GET(SpawnManagerClass* const, pThis, ESI);
	GET(WeaponTypeClass* const, pWeaponType, EAX);
	GET(int, idx, EBX);

	auto const pSpawner = pThis->Owner;
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pSpawner->GetTechnoType());

	if (pTypeExt && pTypeExt->MissileSpawnUseOtherFLHs)
	{
		int burst = pWeaponType->Burst;
		pSpawner->CurrentBurstIndex = idx % burst;
		return SkipCurrentBurstReset;
	}

	return 0;
}

#pragma endregion

#pragma region KeepTargetOnMove

// Do not explicitly reset target for KeepTargetOnMove vehicles when issued move command.
DEFINE_HOOK(0x4C7462, EventClass_Execute_KeepTargetOnMove, 0x5)
{
	enum { SkipGameCode = 0x4C74C0 };

	GET(EventClass*, pThis, ESI);
	GET(TechnoClass*, pTechno, EDI);
	GET(AbstractClass*, pTarget, EBX);

	if (pTechno->WhatAmI() != AbstractType::Unit)
		return 0;

	auto const mission = static_cast<Mission>(pThis->MegaMission.Mission);
	auto const pExt = TechnoExt::ExtMap.Find(pTechno);

	if ((mission == Mission::Move) && pExt->TypeExtData->KeepTargetOnMove && pTechno->Target && !pTarget)
	{
		if (pTechno->IsCloseEnoughToAttack(pTechno->Target))
		{
			auto const pDestination = pThis->MegaMission.Destination.As_Abstract();
			pTechno->SetDestination(pDestination, true);
			pExt->KeepTargetOnMove = true;

			return SkipGameCode;
		}
	}

	pExt->KeepTargetOnMove = false;

	return 0;
}

// Reset the target if beyond weapon range.
// This was originally in UnitClass::Mission_Move() but because that
// is only checked every ~15 frames, it can cause responsiveness issues.
DEFINE_HOOK(0x736480, UnitClass_AI_KeepTargetOnMove, 0x6)
{
	GET(UnitClass*, pThis, ESI);

	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	if (pExt->KeepTargetOnMove && pExt->TypeExtData->KeepTargetOnMove && pThis->Target && pThis->CurrentMission == Mission::Move)
	{
		int weaponIndex = pThis->SelectWeapon(pThis->Target);

		if (auto const pWeapon = pThis->GetWeapon(weaponIndex)->WeaponType)
		{
			int extraDistance = static_cast<int>(pExt->TypeExtData->KeepTargetOnMove_ExtraDistance.Get());
			int range = pWeapon->Range;
			pWeapon->Range += extraDistance; // Temporarily adjust weapon range based on the extra distance.

			if (!pThis->IsCloseEnough(pThis->Target, weaponIndex))
				pThis->SetTarget(nullptr);

			pWeapon->Range = range;
		}
	}

	return 0;
}

#pragma endregion
