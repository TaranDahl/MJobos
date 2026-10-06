
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
