#include "Body.h"

#include <BulletTypeClass.h>

#pragma region UnitsFacing

// Would it be better to rewrite the entire UpdateRotation() ?
DEFINE_HOOK(0x7369A5, UnitClass_UpdateRotation_CheckTurnToTarget, 0x6)
{
	enum { SkipGameCode = 0x736A8E, ContinueGameCode = 0x7369B3 };

	GET(UnitClass* const, pThis, ESI);

	if (!pThis->unknown_bool_6AF)
		return ContinueGameCode;

	if (TechnoExt::ExtData* const pExt = TechnoExt::ExtMap.Find(pThis))
	{
		if (pExt->UnitIdleActionTimer.IsTicking() || pExt->UnitIdleActionGapTimer.IsTicking() || pExt->UnitIdleIsSelected)
			return ContinueGameCode;
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x7369D6, UnitClass_UpdateRotation_StopUnitIdleAction, 0xA)
{
	enum { SkipGameCode = 0x736A8E };

	GET(UnitClass* const, pThis, ESI);

	TechnoTypeExt::ExtData* const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	if (WeaponStruct* const pWeaponStruct = pThis->GetTurretWeapon())
	{
		if (WeaponTypeClass* const pWeapon = pWeaponStruct->WeaponType)
		{
			if (TechnoExt::ExtData* const pExt = TechnoExt::ExtMap.Find(pThis))
				pExt->StopIdleAction();

			if (!pWeapon->OmniFire)
			{
				if (pWeaponStruct->TurretLocked)
				{
					pThis->SecondaryFacing.SetDesired(pThis->PrimaryFacing.Current());
				}
				else
				{
					const CoordStruct source = pThis->Location;
					const CoordStruct target = pThis->Target->GetCoords(); // Checked
					double radian = Math::atan2(source.Y - target.Y, target.X - source.X);

					if (pTypeExt)
						radian += pTypeExt->FacingDeflection * (Math::Pi / 180.0);

					const DirStruct tgtDir { radian };
					pThis->SecondaryFacing.SetDesired(tgtDir);
				}
			}
		}
	}

	if (pTypeExt && pTypeExt->StraightenBody)
	{
		const CoordStruct source = pThis->Location;
		const CoordStruct target = pThis->Target->GetCoords();
		const DirStruct tgtDir { Math::atan2(source.Y - target.Y, target.X - source.X) + (pTypeExt->OrientDeflection * (Math::Pi / 180.0)) };
		pThis->PrimaryFacing.SetDesired(tgtDir);
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x736AFB, UnitClass_UpdateRotation_CheckTurnToForward, 0x6)
{
	enum { SkipGameCode = 0x736BE2, ContinueGameCode = 0x736B21 };

	GET(UnitClass* const, pThis, ESI);

	// Repeatedly judging TurretSpins and IsRotating() is unnecessary
	pThis->unknown_bool_6AF = true;

	if (TechnoExt::ExtData* const pExt = TechnoExt::ExtMap.Find(pThis))
	{
		if (pExt->UnitIdleActionTimer.IsTicking() || pExt->UnitIdleActionGapTimer.IsTicking() || pExt->UnitIdleIsSelected)
			return ContinueGameCode;
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x736B7E, UnitClass_UpdateRotation_ApplyUnitIdleAction, 0xA)
{
	enum { SkipGameCode = 0x736BE2 };

	GET(UnitClass* const, pThis, ESI);

	WeaponStruct* const pWeaponStruct = pThis->GetTurretWeapon();
	TechnoExt::ExtData* const pExt = TechnoExt::ExtMap.Find(pThis);
	const Mission currentMission = pThis->CurrentMission;

	if ((pWeaponStruct && pWeaponStruct->WeaponType && pWeaponStruct->TurretLocked) || (currentMission == Mission::Harmless && pThis->Owner == HouseClass::FindSpecial()))
	{
		// Vanilla TurretLocked state and driver been killed state
		if (pExt)
			pExt->StopIdleAction();

		pThis->SecondaryFacing.SetDesired(pThis->PrimaryFacing.Current());
	}
	else
	{
		// Point to mouse
		if (pExt && pExt->UnitIdleActionSelected && pThis->Owner->IsControlledByCurrentPlayer())
			pExt->ManualIdleAction();

		if (!pExt->UnitIdleIsSelected)
		{
			if (!pThis->Destination)
			{
				// Idle main
				if (pExt && pExt->UnitIdleAction && (currentMission == Mission::Guard || currentMission == Mission::Sticky))
					pExt->ApplyIdleAction();
				else
					pThis->SecondaryFacing.SetDesired(pThis->PrimaryFacing.Current());
			}
			else
			{
				// Turn to destination
				if (pExt)
					pExt->StopIdleAction();

				const CoordStruct source = pThis->Location;
				const CoordStruct target = pThis->Destination->GetCoords();
				double radian = Math::atan2(source.Y - target.Y, target.X - source.X);

				if (TechnoTypeExt::ExtData* const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type))
					radian += pTypeExt->FacingDeflection * (Math::Pi / 180.0);

				const DirStruct tgtDir { radian };
				pThis->SecondaryFacing.SetDesired(pThis->GetTargetDirection(pThis->Destination));
			}
		}
	}

	return SkipGameCode;
}

#pragma endregion

#pragma region CheckFacing

DEFINE_HOOK(0x7410BB, UnitClass_GetFireError_CheckFacingError, 0x8)
{
	enum { NoNeedToCheck = 0x74132B, ContinueCheck = 0x7410C3 };

	GET(FireError, fireError, EAX);

	if (fireError == FireError::OK)
		return ContinueCheck;

	GET(UnitClass*, pThis, ESI);

	return (fireError == FireError::REARM && !pThis->Type->Turret) ? ContinueCheck : NoNeedToCheck;
}

DEFINE_HOOK(0x7412BB, UnitClass_GetFireError_CheckFacingDeviation, 0x7)
{
	enum { SkipGameCode = 0x7412D4 };

	GET(UnitClass* const, pThis, ESI);
	GET(AbstractClass* const, pTarget, EBP);
	GET(BulletTypeClass* const, pBulletType, EDX);
	GET(DirStruct*, tgtDir, EAX);

	const CoordStruct source = pThis->Location;
	const CoordStruct target = pTarget->GetCoords();
	double radian = Math::atan2(source.Y - target.Y, target.X - source.X);

	if (TechnoTypeExt::ExtData* const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type))
		radian += pTypeExt->FacingDeflection * (Math::Pi / 180.0);

	*tgtDir = DirStruct { radian };

	R->EBX(pBulletType->ROT ? 16 : 8);
	R->EAX(tgtDir);
	return SkipGameCode;
}

#pragma endregion
