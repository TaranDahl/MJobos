#include <Ext/Scenario/Body.h>
#include <Ext/TerrainType/Body.h>
#include <Utilities/Macro.h>

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
