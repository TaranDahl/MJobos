#include <AircraftClass.h>
#include "Body.h"
#include <ScenarioClass.h>
#include <TunnelLocomotionClass.h>
#include <Ext/BuildingType/Body.h>
#include <Ext/House/Body.h>
#include <Ext/Anim/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Utilities/EnumFunctions.h>

DEFINE_HOOK(0x73E730, UnitClass_MissionHarvest_HarvesterScanAfterUnload, 0x5)
{
	GET(UnitClass* const, pThis, EBP);
	GET(AbstractClass* const, pFocus, EAX);

	// Focus is set when the harvester is fully loaded and go home.
	if (pFocus && RulesExt::Global()->HarvesterScanAfterUnload)
	{
		CellStruct cellBuffer = { 0,0 };
		CellStruct* pCellStru = (CellStruct*)pThis->ScanForTiberium((DWORD)&cellBuffer, RulesClass::Instance->TiberiumLongScan / 256, 0);

		if (*pCellStru != CellStruct::Empty)
		{
			auto pCell = MapClass::Instance->GetCellAt(*pCellStru);
			int distFromTiberium = pCell ? pThis->DistanceFrom(pCell) : -1;
			int distFromFocus = pThis->DistanceFrom(pFocus);

			// Check if pCell is better than focus.
			if (distFromTiberium > 0 && distFromTiberium < distFromFocus)
				R->EAX(pCell);
		}
	}

	return 0;
}

void __fastcall ArrivingRefineryNearBy(UnitClass* pThis, BuildingClass* pDock)
{
	pDock->UpdateRefinerySmokeSystems();
	reinterpret_cast<int(__thiscall*)(BuildingClass*, int, int, bool, int)>(0x451750)(pDock, 7, !pDock->IsGreenHP(), 0, 0); //BuildingClass::PlayAnimByIdx
	reinterpret_cast<int(__thiscall*)(BuildingClass*, int, int, bool, int)>(0x451750)(pDock, 8, !pDock->IsGreenHP(), 0, 0);

	HouseClass* const pOwner = pDock->GetOwningHouse();

	if (pThis->Type->Weeder)
	{
		bool playAnim = true;

		while (true)
		{
			const int idx = reinterpret_cast<int(__fastcall*)(StorageClass*)>(0x6C9820)(&pThis->Tiberium);

			if (idx == -1)
				break;

			float amount = pThis->Tiberium.GetAmount(idx);
			amount = pThis->Tiberium.RemoveAmount(amount, idx);

			if (amount <= 0.0)
				continue;

			playAnim = false;
			reinterpret_cast<int(__thiscall*)(HouseClass*, int, int)>(0x4F9700)(pOwner, static_cast<int>(amount), idx);
		}

		if (playAnim && pDock->Anims[10])
		{
			pDock->DestroyNthAnim(BuildingAnimSlot::Special);
		}
	}
	else
	{
		int numPurifier = pOwner->NumOrePurifiers;

		if (!pOwner->IsHumanPlayer && SessionClass::Instance->GameMode != GameMode::Campaign)
			numPurifier = RulesClass::Instance->AIVirtualPurifiers.Items[static_cast<int>(pOwner->AIDifficulty)] + numPurifier;

		const float multiplier = numPurifier * RulesClass::Instance->PurifierBonus;
		int money = 0;

		while (true)
		{
			const int idx = reinterpret_cast<int(__fastcall*)(StorageClass*)>(0x6C9820)(&pThis->Tiberium);

			if (idx == -1)
				break;

			float amount = pThis->Tiberium.GetAmount(idx);
			amount = pThis->Tiberium.RemoveAmount(amount, idx);

			if (amount <= 0.0)
				continue;

			money += static_cast<int>(amount * TiberiumClass::Array->Items[idx]->Value);
			const float amountFromPurifier = amount * multiplier;

			if (amountFromPurifier > 0.0)
				money += static_cast<int>(amountFromPurifier * TiberiumClass::Array->Items[idx]->Value);
		}

		if (money)
		{
			pOwner->GiveMoney(money);

			if (BuildingTypeExt::ExtData* const pDockTypeExt = BuildingTypeExt::ExtMap.Find(pDock->Type))
			{
				RulesExt::ExtData* const pRulesExt = RulesExt::Global();

				if ((pRulesExt->DisplayIncome_AllowAI || pDock->Owner->IsControlledByHuman())
					&& pDockTypeExt->DisplayIncome.Get(pRulesExt->DisplayIncome))
				{
					FlyingStrings::AddMoneyString(
						money,
						pDock->Owner,
						pDockTypeExt->DisplayIncome_Houses.Get(pRulesExt->DisplayIncome_Houses.Get()),
						pDock->GetRenderCoords(),
						pDockTypeExt->DisplayIncome_Offset
					);
				}
			}

			// pThis->Animation.Value = 0;
		}
		else if (pDock->Anims[10])
		{
			pDock->DestroyNthAnim(BuildingAnimSlot::Special);
		}
	}

	pThis->MissionStatus = 0;
	pThis->SetDestination(nullptr, false);
}

// MissionStatus == 2 means the unit is returning.
DEFINE_HOOK(0x73EB2C, UnitClass_MissionHarvest_Status2, 0x6)
{
	enum { SkipGameCode = 0x73EE85 };

	GET(UnitClass* const, pThis, EBP);

	UnitTypeClass* const pType = pThis->Type;
	TechnoTypeExt::ExtData* const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	if (!pTypeExt || !pTypeExt->HarvesterQuickUnloader)
		return 0;

	++Unsorted::IKnowWhatImDoing;
	auto const pDock = (BuildingClass*)pThis->vt_entry_528(&pType->Dock, 0, 1); // FindDockInList
	--Unsorted::IKnowWhatImDoing;

	if (!pDock)
		return SkipGameCode;

	CellClass* const pThisCell = pThis->GetCell();

	for (int i = 0; i < 4; ++i)
	{
		if (pThisCell->GetNeighbourCell(static_cast<FacingType>(i << 1))->GetBuilding() == pDock)
		{
			ArrivingRefineryNearBy(pThis, pDock);
			return SkipGameCode;
		}
	}

	const CoordStruct dockLocation = pDock->GetCoords();
	const CoordStruct thisLocation = pThis->GetCoords();

	if (pThis->Destination || (!pType->Teleporter && thisLocation.DistanceFrom(dockLocation) < 768))
		return SkipGameCode;

	const CellStruct destCell = MapClass::Instance->NearByLocation(CellClass::Coord2Cell(dockLocation), // select a nearby cell
		pType->SpeedType, -1, pType->MovementZone, false, 1, 1, false, false, false, false, CellStruct::Empty, false, false);

	if (destCell == CellStruct::Empty)
	{
		pThis->SetDestination(nullptr, true);
		return SkipGameCode;
	}

	CellClass* const pDestCell = MapClass::Instance->GetCellAt(destCell);

	if (!pType->Teleporter)
	{
		pThis->SetDestination(pDestCell, true);
		return SkipGameCode;
	}

	if (FootClass* const ParasiteEatingMe = pThis->ParasiteEatingMe)
		ParasiteEatingMe->ParasiteImUsing->ExitUnit();

	pThis->Mark(MarkType::Up);
	int ChronoOutSound = pType->ChronoOutSound;

	if (ChronoOutSound != -1 || (ChronoOutSound = RulesClass::Instance->ChronoOutSound, ChronoOutSound != -1))
		VocClass::PlayAt(ChronoOutSound, thisLocation, 0);

	if (AnimTypeClass* const pWarpIn = pTypeExt->WarpIn.Get(RulesClass::Instance->WarpIn))
		GameCreate<AnimClass>(pWarpIn, pThis->Location, 0, 1)->Owner = pThis->Owner;

	pThis->SetLocation(pDestCell->GetCoords());
	pThis->OnBridge = pDestCell->ContainsBridge();
	pThis->SetHeight(0);
	pThis->Mark(MarkType::Down);
	pThis->UpdatePosition(PCPType::End);
	pThis->Locomotor->Stop_Moving();
	// pThis->SetDestination(nullptr, true);
	pDestCell->CollectCrate(pThis);
	pThis->unknown_280 = 0;

	int ChronoInSound = pType->ChronoInSound;

	if (ChronoInSound != -1 || (ChronoInSound = RulesClass::Instance->ChronoInSound, ChronoInSound != -1))
		VocClass::PlayAt(ChronoInSound, pThis->Location, 0);

	if (AnimTypeClass* const pWarpOut = pTypeExt->WarpOut.Get(RulesClass::Instance->WarpOut))
		GameCreate<AnimClass>(pWarpOut, pThis->Location, 0, 1)->Owner = pThis->Owner;

	return SkipGameCode;
}
