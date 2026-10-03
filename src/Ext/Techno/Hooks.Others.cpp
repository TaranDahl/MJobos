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
#pragma region ExtraTargeting

static inline bool ExtraTargeting(TechnoClass* pThis, bool area = false)
{
	if (!RulesExt::Global()->ExtraTargeting
		|| pThis->IsALoaner
		|| pThis->SpawnOwner
		|| !pThis->Owner->IsControlledByHuman()
		|| pThis->PlanningToken
		|| TechnoTypeExt::Fetch(pThis->GetTechnoType())->ExtraTargeting_Excluded)
	{
		return false;
	}

	auto coord = (area && pThis->ArchiveTarget ? pThis->ArchiveTarget : pThis)->GetCoords();
	pThis->ShouldLoseTargetNow = true;
	const bool hasTarget = pThis->TargetAndEstimateDamage(coord, area ? ThreatType::Area : ThreatType::Range);
	pThis->ShouldLoseTargetNow = hasTarget;

	return hasTarget;
}

// 按s时
DEFINE_HOOK(0x4C7655, EventClass_RespondToEvent_ExtraTargeting_Idle, 0x7)
{
	enum { SkipGameCode = 0x4C765C };

	GET(TechnoClass*, pTechno, ESI);

	ExtraTargeting(pTechno);

	R->EAX(pTechno->WhatAmI());
	return SkipGameCode;
}

// 步兵载具执行AttackMove的攻击任务且目标死亡时
DEFINE_HOOK(0x4D4E72, FootClass_MissionAttack_ExtraTargeting, 0x6)
{
	enum { ApproachTarget = 0x4D4E64 };

	GET(FootClass*, pThis, ESI);

	return pThis->MegaMissionIsAttackMove() && ExtraTargeting(pThis) ? ApproachTarget : 0;
}

// 建筑开火中且目标死亡时
DEFINE_HOOK(0x44AF90, BuildingClass_MissionAttack_ExtraTargeting, 0x5)
{
	enum { AttackTarget = 0x44AFED };

	GET(BuildingClass*, pThis, ESI);

	return ExtraTargeting(pThis) ? AttackTarget : 0;
}

// 飞机执行AttackMove的攻击任务且目标死亡时
DEFINE_HOOK(0x417FE0, AircraftClass_MissionAttack_ExtraTargeting, 0x6)
{
	GET(AircraftClass*, pThis, ECX);

	if (!pThis->Target && pThis->MegaMissionIsAttackMove())
		ExtraTargeting(pThis);

	return 0;
}

// 具有OpportunityFire的单位在接收到攻击和区域警戒之外的鼠标指令时
DEFINE_HOOK(0x4C7462, EventClass_RespondToEvent_ExtraTargeting_MegaMission, 0x5)
{
	enum { SkipGameCode = 0x4C74C0, SkipSetTarget = 0x4C746D };

	GET(TechnoClass*, pTechno, EDI);
	GET(EventClass*, pThis, ESI);
	GET(AbstractClass*, pTarget, EBX);

	auto const mission = static_cast<Mission>(pThis->MegaMission.Mission);

	if (const auto pUnit = abstract_cast<UnitClass*, true>(pTechno))
	{
		auto const pExt = UnitExt::Fetch(pUnit);

		if (mission == Mission::Move)
		{
			// Explicitly reset subterranean harvester state machine.
			pExt->SubterraneanHarvStatus = 0;
			pExt->SubterraneanHarvRallyPoint = nullptr;

			// Do not explicitly reset target for KeepTargetOnMove vehicles when issued move command.
			if (pExt->TypeExtData->KeepTargetOnMove && pTechno->Target)
			{
				if (!pTarget && pTechno->IsCloseEnoughToAttack(pTechno->Target))
				{
					auto const pDestination = pThis->MegaMission.Destination.As_Abstract();
					pTechno->SetDestination(pDestination, true);
					pExt->KeepTargetOnMove = true;

					return SkipGameCode;
				}
			}
		}

		pExt->KeepTargetOnMove = false;
	}

	if (pTarget || !pTechno->GetTechnoType()->OpportunityFire)
		return 0;

	auto currentMission = pTechno->GetCurrentMission();

	if (currentMission == Mission::Attack || currentMission == Mission::Area_Guard || !ExtraTargeting(pTechno))
	{
		// These missions won't change to the queued mission if the techno has target.
		pTechno->TargetingTimer.Stop();
		return 0;
	}

	return SkipSetTarget;
}

DEFINE_HOOK(0x709918, TechnoClass_TargetAndEstimateDamage_CheckTarget, 0x6)
{
	enum { CanTargeting = 0x709926 };

	GET(TechnoClass* const, pThis, ESI);

	return RulesExt::Global()->ExtraTargeting
		&& pThis->QueuedMission != Mission::Attack
		&& (pThis->WhatAmI() != AbstractType::Unit
			|| !UnitExt::Fetch(static_cast<UnitClass*>(pThis))->KeepTargetOnMove)
		&& pThis->Owner->IsControlledByHuman()
		? CanTargeting
		: 0;
}

DEFINE_HOOK(0x709957, TechnoClass_TargetAndEstimateDamage_SetTarget, 0x6)
{
	enum { SkipSetTarget = 0x709966, SkipSetTargetAndEstimateHealth = 0x7099B8 };

	GET(TechnoClass*, pThis, ESI);
	GET(AbstractClass*, pTarget, EDI);

	if (pTarget && (!RulesExt::Global()->ExtraTargeting || (pThis->QueuedMission != Mission::Attack && pThis->Target != pTarget)))
		pThis->SetTarget(pTarget);

	return SkipSetTarget;
}

// 受到伤害时
DEFINE_HOOK(0x702B31, TechnoClass_ReceiveDamage_DoRetaliate, 0x7)
{
	enum { SkipGameCode = 0x702B47 };

	GET(TechnoClass*, pThis, ESI);

	if (RulesExt::Global()->ExtraTargeting && pThis->Owner->IsControlledByHuman())
	{
		auto mission = pThis->GetCurrentMission();

		// AttackMove
		if (mission == Mission::Attack)
		{
			if (pThis->MegaMissionIsAttackMove())
				ExtraTargeting(pThis);
		}
		// Other auto-target-able missions
		else if (mission == Mission::Harvest || mission == Mission::Move || mission == Mission::Guard || mission == Mission::Area_Guard)
		{
			// If pThis has target, do targeting, else reset timer and it will do targeting next frame.
			if (pThis->Target)
				ExtraTargeting(pThis, mission == Mission::Area_Guard);
			else
				pThis->TargetingTimer.Stop();
		}

		return SkipGameCode;
	}

	return 0;
}

#pragma endregion
