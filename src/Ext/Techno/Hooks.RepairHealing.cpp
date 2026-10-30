#include <HouseClass.h>
#include <SessionClass.h>
#include <TechnoClass.h>

#include <Utilities/Macro.h>

#pragma region FixRepairDistance

DEFINE_PATCH(0x44C70B, 0xC8);
DEFINE_JUMP(LJMP, 0x44C75E, 0x44C793);

#pragma endregion

#pragma region HealingWeaponFix

// Skip the hardcoded healing weapon auto-target range.
DEFINE_JUMP(LJMP, 0x6F9024, 0x6F9042);

DEFINE_HOOK(0x6FA9D8, TechnoClass_Update_FixRepairWeapon, 0x6)
{
	enum { SkipGameCode = 0x6FAA6F };

	GET(TechnoClass*, pThis, ESI);

	if (pThis->Target && pThis->CombatDamage(-1) < 0)
	{
		if ((SessionClass::IsCampaign()
			? pThis->Owner->IsControlledByCurrentPlayer()
			: pThis->Owner->IsHumanPlayer)
			&& !pThis->Owner->IsAlliedWith(pThis->Target))
		{
			pThis->SetTarget(nullptr);
		}
		else if (pThis->CurrentMission == Mission::Guard && !pThis->IsCloseEnoughToAttack(pThis->Target))
		{
			pThis->SetTarget(nullptr);
		}
	}

	return SkipGameCode;
}

#pragma endregion
