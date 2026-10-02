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

#pragma region DebugLogInGreatestThreat

DEFINE_HOOK(0x6F9C80, TechnoClass_GreatestThreat_LogDeadInTechnoArray, 0x6)
{
	enum { Continue = 0x6F9C89, NextOne = 0x6F9D93 };

	GET(TechnoClass* const, pThis, ESI);
	GET(int, index, EBX);

	const auto pTechno = TechnoClass::Array.Items[index];
	const bool safe = (VTable::Get(pTechno) & 0xFFFE0000) == 0x7E0000;

	if (!safe || pTechno->IsDead())
	{
		if (!safe || VTable::Get(pTechno) == 0x7E1F50) // AbstractClass::AbsVTable
		{
			Debug::LogAndMessage("TechnoClass::GreatestThreat: Found DeadTechno(0x%08X) with dirty vtable in TechnoArray!\n",
				reinterpret_cast<DWORD>(pTechno));

			TechnoClass::Array.RemoveItem(index);
			R->EBX(index - 1);

			if (SessionClass::IsSingleplayer() && Phobos::Config::DevelopmentCommands)
			{
				Debug::LogAndMessage("Skip processing. Entering Stepping Mode...\n");
				FrameByFrameCommandClass::FrameStep = true;
				auto coords = pThis->GetCoords();
				TacticalClass::Instance->SetTacticalPosition(&coords);
			}
			else
			{
				Debug::LogAndMessage("Skip processing.\n");
			}
		}
		else
		{
			Debug::Log("TechnoClass::GreatestThreat: Found DeadTechno(0x%08X)[%s]at(%d,%d) in TechnoArray.\n",
				reinterpret_cast<DWORD>(pTechno), pTechno->get_ID(), (pThis->Location.X / 256), (pThis->Location.Y / 256));
		}

		return NextOne;
	}

	R->ECX(pThis->Owner);
	R->EDI(pTechno);
	return Continue;
}

DEFINE_HOOK(0x6F91EC, TechnoClass_GreatestThreat_LogDeadInAircraftTracker, 0x6)
{
	enum { NextOne = 0x6F9377 };

	GET(TechnoClass* const, pThis, ESI);
	GET(TechnoClass* const, pTechno, EBP);

	const bool safe = (VTable::Get(pTechno) & 0xFFFE0000) == 0x7E0000;

	if (!safe || pTechno->IsDead())
	{
		if (!safe || VTable::Get(pTechno) == 0x7E1F50) // AbstractClass::AbsVTable
		{
			Debug::LogAndMessage("TechnoClass::GreatestThreat: Found DeadTechno(0x%08X) with dirty vtable in AircraftTracker!\n",
				reinterpret_cast<DWORD>(pTechno));

			auto removeFromATC = [pTechno]()
			{
				for (int i = 0; i < 20; ++i)
				{
					for (int j = 0; j < 20; ++j)
					{
						if (AircraftTrackerClass::Instance.TrackerVectors[i][j].Remove(pTechno))
							return;
					}
				}
			};
			removeFromATC();

			if (SessionClass::IsSingleplayer() && Phobos::Config::DevelopmentCommands)
			{
				Debug::LogAndMessage("Skip processing. Entering Stepping Mode...\n");
				FrameByFrameCommandClass::FrameStep = true;
				auto coords = pThis->GetCoords();
				TacticalClass::Instance->SetTacticalPosition(&coords);
			}
			else
			{
				Debug::LogAndMessage("Skip processing.\n");
			}
		}
		else
		{
			Debug::Log("TechnoClass::GreatestThreat: Found DeadTechno(0x%08X)[%s]at(%d,%d) in AircraftTracker.\n",
				reinterpret_cast<DWORD>(pTechno), pTechno->get_ID(), (pThis->Location.X / 256), (pThis->Location.Y / 256));
		}

		return NextOne;
	}

	return 0;
}

#pragma endregion

#pragma region DebugLogInTetherCheck

DEFINE_HOOK(0x7043B9, TechnoClass_GetZAdjustment_LogTetherButNoLink, 0x6)
{
	enum { SkipGameCode = 0x7043E1 };

	GET(TechnoClass* const, pLink, EAX);

	if (pLink)
		return 0;

	GET(TechnoClass* const, pThis, ESI);

	Debug::LogAndMessage("TechnoClass::GetZAdjustment: Found a Techno [%s] in Tether state without any RadioLink!\n", pThis->get_ID());

	pThis->IsTether = false;

	if (SessionClass::IsSingleplayer() && Phobos::Config::DevelopmentCommands)
	{
		Debug::LogAndMessage("Skip processing. Entering Stepping Mode...\n");
		FrameByFrameCommandClass::FrameStep = true;
		auto coords = pThis->GetCoords();
		TacticalClass::Instance->SetTacticalPosition(&coords);
	}
	else
	{
		Debug::LogAndMessage("Skip processing.\n");
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x73B0C5, UnitClass_DrawIfVisible_LogTetherButNoLink, 0x6)
{
	enum { SkipGameCode = 0x73B124 };

	GET(TechnoClass* const, pLink, EAX);

	if (pLink)
		return 0;

	GET(TechnoClass* const, pThis, EDI);

	Debug::LogAndMessage("UnitClass::DrawIfVisible: Found a Unit [%s] in Tether state without any RadioLink!\n", pThis->get_ID());

	pThis->IsTether = false;

	if (SessionClass::IsSingleplayer() && Phobos::Config::DevelopmentCommands)
	{
		Debug::LogAndMessage("Skip processing. Entering Stepping Mode...\n");
		FrameByFrameCommandClass::FrameStep = true;
		auto coords = pThis->GetCoords();
		TacticalClass::Instance->SetTacticalPosition(&coords);
	}
	else
	{
		Debug::LogAndMessage("Skip processing.\n");
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x7410D6, UnitClass_GetFireError_LogTetherButNoLink, 0x7)
{
	enum { SkipGameCode = 0x7410EC };

	GET(TechnoClass* const, pLink, EAX);

	if (pLink)
		return 0;

	GET(TechnoClass* const, pThis, ESI);

	Debug::LogAndMessage("UnitClass::GetFireError: Found a Unit [%s] in Tether state without any RadioLink!\n", pThis->get_ID());

	pThis->IsTether = false;

	if (SessionClass::IsSingleplayer() && Phobos::Config::DevelopmentCommands)
	{
		Debug::LogAndMessage("Skip processing. Entering Stepping Mode...\n");
		FrameByFrameCommandClass::FrameStep = true;
		auto coords = pThis->GetCoords();
		TacticalClass::Instance->SetTacticalPosition(&coords);
	}
	else
	{
		Debug::LogAndMessage("Skip processing.\n");
	}

	return SkipGameCode;
}

#pragma endregion

// TODO Debug hooks
