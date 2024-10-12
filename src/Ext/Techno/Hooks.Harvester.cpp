#include <AircraftClass.h>
#include "Body.h"

#include <ScenarioClass.h>
#include <TunnelLocomotionClass.h>

#include <Ext/BuildingType/Body.h>
#include <Ext/House/Body.h>
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
			{
				R->EAX(pCell);
			}
		}
	}

	return 0;
}

#pragma region HarvesterQuickUnload

/*
// MissionStatus == 2 means the unit is returning.
DEFINE_HOOK(0x, UnitClass_MissionHarvest_Status2, 0x)
{

}

DEFINE_HOOK(0x73E755, TEST, 0x6)
{
	GET(UnitClass*, pThis, EBP);

	if (!pThis->Destination)
		return 0;

	auto ParasiteEatingMe = pThis->ParasiteEatingMe;
	if (ParasiteEatingMe)
		ParasiteEatingMe->ParasiteImUsing->ExitUnit();
	pThis->Mark(MarkType::Up);
	auto v28 = pThis;
	auto ChronoOutSound = v28->GetTechnoType()->ChronoOutSound;
	CoordStruct pCoord;
	auto LastCoords = pThis->Destination->GetCoords();
	if (ChronoOutSound != -1 || (ChronoOutSound = RulesClass::Instance->ChronoOutSound, ChronoOutSound != -1))
	{
		pCoord = v28->Location;
		VocClass::PlayAt(ChronoOutSound, pCoord, 0);
	}
	CellStruct cell = { LastCoords.X / 256, LastCoords.Y / 256 };
	auto v46 = cell;
	auto CellAt = MapClass::Instance->GetCellAt(v46);
	pThis->SetLocation(LastCoords);
	pThis->OnBridge = 0;//(CellAt->Flags & 0x100) != 0;
	pThis->SetHeight(0);
	pThis->Mark(MarkType::Down);
	auto pUnit = pThis;
	auto ChronoInSound = pUnit->GetTechnoType()->ChronoInSound;
	if (ChronoInSound != -1 || (ChronoInSound = RulesClass::Instance->ChronoInSound, ChronoInSound != -1))
	{
		pCoord = pUnit->Location;
		VocClass::PlayAt(ChronoInSound, pCoord, 0);
	}
	pThis->UpdatePosition(PCPType::End);
	pThis->SetDestination(0, 1);
	pThis->Locomotor->Stop_Moving();
	CellClass::CollectCrate(CellAt, pThis);
	v33 = operator new(0x1C8u);
	if (v33)
	{
		pCoord = pThis->Location;
		AnimClass::CTOR(v33, RulesClass::Instance->WarpOut, &pCoord, 0, 1, 1536, 0, 0);
	}
	pThis->unknown_280 = 0;

	return 0;
}

*/

#pragma endregion

