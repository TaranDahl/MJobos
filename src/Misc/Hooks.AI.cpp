#include <New/Entity/AttachmentClass.h>

#include <Ext/Scenario/Body.h>
#include <Ext/TerrainType/Body.h>
#include <Utilities/Macro.h>

#pragma region SmudgeUpdate

static bool __forceinline ShouldRemoveSmudgeCell(const int index, const int time, const int current)
{
	const auto cell = CellStruct{static_cast<short>(index & 511), static_cast<short>(index >> 9) };

	if (const auto pCell = MapClass::Instance.TryGetCellAt(cell))
	{
		if (pCell->SmudgeTypeIndex != -1)
		{
			const auto pCellExt = CellExt::Fetch(pCell);

			if ((pCellExt->SmudgeGenerate + time) > current)
				return false;

			const auto state = pCellExt->SmudgeState;

			if (state != BlitterFlags::TransLucent75)
			{
				pCellExt->SmudgeGenerate = current;
				pCellExt->SmudgeState = static_cast<BlitterFlags>(static_cast<size_t>(state) + 2u);
				pCell->MarkForRedraw();
				return false;
			}

			pCell->SmudgeTypeIndex = -1;
			pCell->MarkForRedraw();
		}
	}

	return true;
}

DEFINE_HOOK(0x55B6B3, LogicClass_AI_After, 0x5)
{
	for (auto const& attachment : AttachmentClass::Array)
		attachment->AI();

	const int time = RulesExt::Global()->SmudgeUpdateTime;

	if (time > 0)
	{
		auto& s = ScenarioExt::Global()->Smudges;

		if (!s.empty())
		{
			const int current = Unsorted::CurrentFrame;

			for (auto it = s.begin(); it != s.end(); )
			{
				if (ShouldRemoveSmudgeCell(*it, time, current))
					it = s.erase(it);
				else
					++it;
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x6B60DE, SmudgeTypeClass_Mark_SetContext, 0x6)
{
	GET(CellClass* const, pCell, EAX);

	ScenarioExt::Global()->Smudges.insert(MapClass::GetCellIndex(pCell->MapCoords));
	const auto pCellExt = CellExt::Fetch(pCell);
	pCellExt->SmudgeGenerate = Unsorted::CurrentFrame;
	pCellExt->SmudgeState = BlitterFlags::None;

	return 0;
}

DEFINE_HOOK(0x6B56AC, SmudgeTypeClass_DrawIt_DrawTrans, 0x5)
{
	GET(CellClass* const, pCell, ESI);
	REF_STACK(BlitterFlags, flags, STACK_OFFSET(0x3C, -0x3C));

	flags |= CellExt::Fetch(pCell)->SmudgeState;

	return 0;
}

#pragma endregion

#pragma region AirBarrier

void __fastcall FindMovingInfOrVeh(CellClass* const pCell, const AbstractType findType)
{
	const auto flag = pCell->OccupationFlags;
	pCell->OccupationFlags = 0;
	auto checkCell = pCell->MapCoords + CellStruct { 2, 2 };

	for (short checkX = checkCell.X - 4; checkX <= checkCell.X; ++checkX)
	{
		for (short checkY = checkCell.Y - 4; checkY <= checkCell.Y; ++checkY)
		{
			const auto pAdjCheckCell = MapClass::Instance.GetCellAt(CellStruct { checkX, checkY });

			for (auto pObject = pAdjCheckCell->FirstObject; pObject; pObject = pObject->NextObject)
			{
				if (pObject->WhatAmI() == findType && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
				{
					pCell->OccupationFlags = flag;
					return;
				}
			}
		}
	}
}

void __fastcall FindMovingInfAndVeh(CellClass* const pCell)
{
	const auto flag = pCell->OccupationFlags;
	pCell->OccupationFlags = 0;
	bool inf = false;
	bool veh = false;
	auto checkCell = pCell->MapCoords + CellStruct { 2, 2 };

	for (short checkX = checkCell.X - 4; checkX <= checkCell.X; ++checkX)
	{
		for (short checkY = checkCell.Y - 4; checkY <= checkCell.Y; ++checkY)
		{
			const auto pAdjCheckCell = MapClass::Instance.GetCellAt(CellStruct { checkX, checkY });

			for (auto pObject = pAdjCheckCell->FirstObject; pObject; pObject = pObject->NextObject)
			{
				const auto absType = pObject->WhatAmI();

				if (absType == AbstractType::Infantry)
				{
					if (!inf && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
					{
						pCell->OccupationFlags |= (flag & 0x1F);

						if (veh)
							return;

						inf = true;
					}
				}
				else if (absType == AbstractType::Unit)
				{
					if (!veh && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
					{
						pCell->OccupationFlags |= (flag & 0x20);

						if (inf)
							return;

						veh = true;
					}
				}
			}
		}
	}
}

void __fastcall FindAltMovingInfOrVeh(CellClass* const pCell, const AbstractType findType)
{
	const auto flag = pCell->AltOccupationFlags;
	pCell->AltOccupationFlags = 0;
	auto checkCell = pCell->MapCoords + CellStruct { 2, 2 };

	for (short checkX = checkCell.X - 4; checkX <= checkCell.X; ++checkX)
	{
		for (short checkY = checkCell.Y - 4; checkY <= checkCell.Y; ++checkY)
		{
			const auto pAdjCheckCell = MapClass::Instance.GetCellAt(CellStruct { checkX, checkY });

			for (auto pObject = pAdjCheckCell->AltObject; pObject; pObject = pObject->NextObject)
			{
				if (pObject->WhatAmI() == findType && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
				{
					pCell->AltOccupationFlags = flag;
					return;
				}
			}
		}
	}
}

void __fastcall FindAltMovingInfAndVeh(CellClass* const pCell)
{
	const auto flag = pCell->AltOccupationFlags;
	pCell->AltOccupationFlags = 0;
	bool inf = false;
	bool veh = false;
	auto checkCell = pCell->MapCoords + CellStruct { 2, 2 };

	for (short checkX = checkCell.X - 4; checkX <= checkCell.X; ++checkX)
	{
		for (short checkY = checkCell.Y - 4; checkY <= checkCell.Y; ++checkY)
		{
			const auto pAdjCheckCell = MapClass::Instance.GetCellAt(CellStruct { checkX, checkY });

			for (auto pObject = pAdjCheckCell->AltObject; pObject; pObject = pObject->NextObject)
			{
				const auto absType = pObject->WhatAmI();

				if (absType == AbstractType::Infantry)
				{
					if (!inf && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
					{
						pCell->AltOccupationFlags |= (flag & 0x1F);

						if (veh)
							return;

						inf = true;
					}
				}
				else if (absType == AbstractType::Unit)
				{
					if (!veh && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
					{
						pCell->AltOccupationFlags |= (flag & 0x20);

						if (inf)
							return;

						veh = true;
					}
				}
			}
		}
	}
}

DEFINE_HOOK(0x55B4E1, LogicClass_Update_UnmarkCellOccupationFlags, 0x5)
{
	const auto delay = RulesExt::Global()->CleanUpAirBarrier.Get();

	if (delay > 0 && !(Unsorted::CurrentFrame % delay))
	{
		auto& pMap = MapClass::Instance;
		pMap.CellIteratorReset();

		for (auto pCell = pMap.CellIteratorNext(); pCell; pCell = pMap.CellIteratorNext())
		{
			if ((0xFF & pCell->OccupationFlags) && !pCell->FirstObject)
			{
				pCell->OccupationFlags &= 0x3F; // ~(Aircraft | Building)

				if (pCell->OccupationFlags & 0x1F)
				{
					if (pCell->OccupationFlags & 0x20)
						FindMovingInfAndVeh(pCell);
					else
						FindMovingInfOrVeh(pCell, AbstractType::Infantry);
				}
				else if (pCell->OccupationFlags & 0x20)
				{
					FindMovingInfOrVeh(pCell, AbstractType::Unit);
				}
			}

			if ((0xFF & pCell->AltOccupationFlags) && !pCell->AltObject)
			{
				pCell->AltOccupationFlags &= 0x3F; // ~(Aircraft | Building)

				if (pCell->AltOccupationFlags & 0x1F)
				{
					if (pCell->AltOccupationFlags & 0x20)
						FindAltMovingInfAndVeh(pCell);
					else
						FindAltMovingInfOrVeh(pCell, AbstractType::Infantry);
				}
				else if (pCell->AltOccupationFlags & 0x20)
				{
					FindAltMovingInfOrVeh(pCell, AbstractType::Unit);
				}
			}
		}
	}

	return 0;
}

#pragma endregion
