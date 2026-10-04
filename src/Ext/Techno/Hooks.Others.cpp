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

#pragma region EventListOverflow

DEFINE_HOOK(0x6FFE00, TechnoClass_ClickedEvent_CacheClickedEvent, 0x5)
{
	GET(TechnoClass*, pThis, ECX);
	GET_STACK(EventType, event, 0x4);

	if (EventClass::OutList.Count >= 128)
	{
		auto const pExt = TechnoExt::Fetch(pThis);
		pExt->HasCachedClickEvent = true;
		pExt->CachedEventType = event;
		// one cache at a time
		pExt->HasCachedClickMission = false;
		pExt->CachedMission = Mission::None;
		pExt->CachedCell = nullptr;
		pExt->CachedTarget = nullptr;
	}

	return 0;
}

DEFINE_HOOK(0x6FFDA5, TechnoClass_ClickedMission_CacheClickedMission, 0x7)
{
	GET_STACK(AbstractClass* const, pCell, STACK_OFFSET(0x98, 0xC));
	GET(TechnoClass* const, pThis, ESI);
	GET(AbstractClass* const, pTarget, EBP);
	GET(Mission const, mission, EDI);

	if (EventClass::OutList.Count >= 128)
	{
		auto const pExt = TechnoExt::Fetch(pThis);
		pExt->HasCachedClickMission = true;
		pExt->CachedMission = mission;
		pExt->CachedCell = pCell;
		pExt->CachedTarget = pTarget;
		// one cache at a time
		pExt->HasCachedClickEvent = false;
		pExt->CachedEventType = EventType::LAST_EVENT;
	}

	return 0;
}

#pragma endregion
