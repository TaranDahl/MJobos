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

// MissionStatus == 2 means the unit is returning.
DEFINE_HOOK(0x73EB2C, UnitClass_MissionHarvest_Status2, 0x6)
{
	enum { FuncBreak = 0x73EE85 };

	GET(UnitClass* const, pThis, EBP);

	auto const pType = pThis->Type;
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	if (!pTypeExt || !pTypeExt->HarvesterQuickUnloader)
		return 0;

	++Unsorted::IKnowWhatImDoing;
	auto const pDock = (BuildingClass*)pThis->vt_entry_528(&pType->Dock, 0, 1); // FindDockInList
	--Unsorted::IKnowWhatImDoing;

	if (!pDock)
		return FuncBreak;

	auto const pDockType = pDock->Type;
	auto const dockCrds = pDock->GetCoords();
	auto const dockCell = pDock->GetMapCoords();
	auto const thisLocation = pThis->Location;
	auto const thisCell = CellClass::Coord2Cell(pThis->GetCoords());
	auto const pThisCell = MapClass::Instance->GetCellAt(thisCell);
	bool isDockNearBy = pThisCell ? pThisCell->GetBuilding() == pDock : false;

	if (!isDockNearBy)
	{
		for (auto pFoundation = pDockType->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; pFoundation++)
		{
			CellStruct searchCell = dockCell + *pFoundation;

			if (searchCell == thisCell)
			{
				isDockNearBy = true;
				break;
			}
		}
	}

	if (isDockNearBy)
	{
		pDock->UpdateRefinerySmokeSystems();
		reinterpret_cast<int(__thiscall*)(BuildingClass*, int, int, bool, int)>(0x451750)(pDock, 7, !pDock->IsGreenHP(), 0, 0); //BuildingClass::PlayAnimByIdx
		reinterpret_cast<int(__thiscall*)(BuildingClass*, int, int, bool, int)>(0x451750)(pDock, 8, !pDock->IsGreenHP(), 0, 0);

		const int idx = reinterpret_cast<int(__fastcall*)(float*)>(0x6C9820)(&pThis->Tiberium.Tiberium1);
		auto const pOwner = pDock->GetOwningHouse();
		const bool isHumanPlayer = pOwner->IsHumanPlayer;
		int numPurifier = pOwner->NumOrePurifiers;

		if (!isHumanPlayer && SessionClass::Instance->GameMode != GameMode::Campaign)
			numPurifier = RulesClass::Instance->AIVirtualPurifiers.Items[static_cast<int>(pOwner->AIDifficulty)] + numPurifier;

		float amount = idx == -1 ? 0.0f : pThis->Tiberium.GetAmount(idx);
		amount = idx == -1 ? 0.0f : pThis->Tiberium.RemoveAmount(amount, idx);
		const float amountFromPurifier = amount * numPurifier * RulesClass::Instance->PurifierBonus;

		if (idx == -1 || amount <= 0.0)
		{
			if (pDock->Anims[10])
				pDock->DestroyNthAnim(BuildingAnimSlot::Special);
		}
		else if (pType->Weeder)
		{
			reinterpret_cast<int(__thiscall*)(HouseClass*, int, int)>(0x4F9700)(pOwner, static_cast<int>(amount), idx);
			// pThis->Animation.Value = 0;
		}
		else
		{
			pOwner->GiveMoney(static_cast<int>(amount * TiberiumClass::Array->Items[idx]->Value));

			if (amountFromPurifier > 0.0)
				pOwner->GiveMoney(static_cast<int>(amountFromPurifier * TiberiumClass::Array->Items[idx]->Value));

			// pThis->Animation.Value = 0;
		}

		pThis->MissionStatus = 0;
		pThis->SetDestination(nullptr, false);

		return FuncBreak;
	}

	if (pType->BalloonHover ? !pThis->Locomotor->Is_Moving_Now() : (pThis->Destination == nullptr))
	{
		pThis->SetDestination(pDock, false); // set to destination

		if (pType->Teleporter)
		{
			auto const destCell = MapClass::Instance->NearByLocation(CellClass::Coord2Cell(dockCrds), // select a nearby cell
				pType->SpeedType, -1, pType->MovementZone, false, 1, 1, false,
				false, false, false, CellStruct::Empty, false, false);
			auto const pDestCell = MapClass::Instance->TryGetCellAt(destCell);
			// I'm too stupid to understand the Locomotors.
			// But I can handle them myself. Works fine seemingly.
			if (thisCell.DistanceFrom(destCell) >= 3.0)
			{
				if (auto const ParasiteEatingMe = pThis->ParasiteEatingMe)
					ParasiteEatingMe->ParasiteImUsing->ExitUnit();

				pThis->Mark(MarkType::Up);
				auto ChronoOutSound = pType->ChronoOutSound;

				if (ChronoOutSound != -1 || (ChronoOutSound = RulesClass::Instance->ChronoOutSound, ChronoOutSound != -1))
					VocClass::PlayAt(ChronoOutSound, thisLocation, 0);

				if (auto const pWarpIn = pTypeExt->WarpIn.Get(RulesClass::Instance->WarpIn))
					GameCreate<AnimClass>(pWarpIn, pThis->Location, 0, 1)->Owner = pThis->Owner;

				pThis->SetLocation(pDestCell->GetCoords());
				pThis->OnBridge = pDestCell->ContainsBridge();
				pThis->SetHeight(0);
				pThis->Mark(MarkType::Down);
				pThis->UpdatePosition(PCPType::End);
				pThis->Locomotor->Stop_Moving();
				// pThis->SetDestination(0, 1);
				pDestCell->CollectCrate(pThis);
				pThis->unknown_280 = 0;

				auto ChronoInSound = pType->ChronoInSound;

				if (ChronoInSound != -1 || (ChronoInSound = RulesClass::Instance->ChronoInSound, ChronoInSound != -1))
					VocClass::PlayAt(ChronoInSound, pThis->Location, 0);

				if (auto const pWarpOut = pTypeExt->WarpOut.Get(RulesClass::Instance->WarpOut))
					GameCreate<AnimClass>(pWarpOut, pThis->Location, 0, 1)->Owner = pThis->Owner;
			}
		}
	}

	return FuncBreak;
}
