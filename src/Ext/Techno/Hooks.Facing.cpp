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

	TechnoTypeExt::ExtData* pTypeExt = nullptr;
	AbstractClass* const pTarget = pThis->Target; // pThis->Target have been checked
	const int weaponIndex = pThis->SelectWeapon(pTarget);

	if (WeaponStruct* const pWeaponStruct = pThis->GetWeapon(weaponIndex)) // Vanilla is pThis->GetTurretWeapon()
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
					const CoordStruct target = pTarget->GetCoords();
					double radian = Math::atan2(source.Y - target.Y, target.X - source.X);
					DirStruct tgtDir;

					if (pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type), pTypeExt)
					{
						const double rotate = pTypeExt->Turret_SelfRotation_Angle * (Math::Pi / 180.0);

						if (pTypeExt->Turret_SelfRotation_Symmetric)
						{
							const DirStruct curDir = pThis->SecondaryFacing.Current();
							const DirStruct rightDir = DirStruct { radian + rotate };
							const DirStruct leftDir = DirStruct { radian - rotate };

							if (abs(static_cast<short>(rightDir.Raw) - static_cast<short>(curDir.Raw)) < abs(static_cast<short>(leftDir.Raw) - static_cast<short>(curDir.Raw)))
								tgtDir = rightDir;
							else
								tgtDir = leftDir;
						}
						else
						{
							tgtDir = DirStruct { radian + rotate };
						}
					}

					pThis->SecondaryFacing.SetDesired(tgtDir);
				}
			}
		}
	}

	if (!pThis->Destination && weaponIndex != -1 && pThis->IsCloseEnough(pTarget, weaponIndex))
	{
		if ((pTypeExt || (pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type), pTypeExt)) && pTypeExt->Turret_BodyRotation_Enable)
		{
			const double rotate = pTypeExt->Turret_BodyRotation_Angle * (Math::Pi / 180.0);
			const DirStruct curDir = pThis->PrimaryFacing.Current();

			const CoordStruct source = pThis->Location;
			const CoordStruct target = pTarget->GetCoords();
			double radian = Math::atan2(source.Y - target.Y, target.X - source.X);
			DirStruct tgtDir;

			if (pTypeExt->Turret_BodyRotation_Symmetric)
			{
				const DirStruct rightDir = DirStruct { radian + rotate };
				const DirStruct leftDir = DirStruct { radian - rotate };

				if (abs(static_cast<short>(rightDir.Raw) - static_cast<short>(curDir.Raw)) < abs(static_cast<short>(leftDir.Raw) - static_cast<short>(curDir.Raw)))
					tgtDir = rightDir;
				else
					tgtDir = leftDir;
			}
			else
			{
				tgtDir = DirStruct { radian + rotate };
			}

			if (abs(static_cast<short>(tgtDir.Raw) - static_cast<short>(curDir.Raw)) >= 8192)
				pThis->PrimaryFacing.SetDesired(tgtDir);
		}
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
				const DirStruct tgtDir { Math::atan2(source.Y - target.Y, target.X - source.X) };
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
	GET(DirStruct, curDir, EDI);
	GET(DirStruct*, pTargetDir, EAX);

	const CoordStruct source = pThis->Location;
	const CoordStruct target = pTarget->GetCoords();
	double radian = Math::atan2(source.Y - target.Y, target.X - source.X);

	if (pThis->Type->Turret)
	{
		if (TechnoTypeExt::ExtData* const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type))
		{
			const double rotate = pTypeExt->Turret_SelfRotation_Angle * (Math::Pi / 180.0);

			if (pTypeExt->Turret_SelfRotation_Symmetric)
			{
				const DirStruct rightDir = DirStruct { radian + rotate };
				const DirStruct leftDir = DirStruct { radian - rotate };

				if (abs(static_cast<short>(rightDir.Raw) - static_cast<short>(curDir.Raw)) < abs(static_cast<short>(leftDir.Raw) - static_cast<short>(curDir.Raw)))
					*pTargetDir = rightDir;
				else
					*pTargetDir = leftDir;
			}
			else
			{
				*pTargetDir = DirStruct { radian + rotate };
			}
		}
		else
		{
			*pTargetDir = DirStruct { radian };
		}
	}
	else
	{
		*pTargetDir = DirStruct { radian };
	}

	R->EBX(pBulletType->ROT ? 16 : 8);
	R->EAX(pTargetDir);
	return SkipGameCode;
}

#pragma endregion
