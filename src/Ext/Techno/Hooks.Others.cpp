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

#pragma region VisualCharacter

namespace VisualCharacterContext
{
	TechnoClass* pThis = nullptr;
	bool specificOwner = false;
	HouseClass* pWatcher = nullptr;
}

// Set context
DEFINE_HOOK(0x703860, TechnoClass_VisualCharacter_Start, 0x6)
{
	GET(TechnoClass* const, pThis, ECX);
	GET_STACK(const bool, specificOwner, STACK_OFFSET(0, 0x4));
	GET_STACK(HouseClass* const, pWatcher, STACK_OFFSET(0, 0x8));

	VisualCharacterContext::pThis = pThis;
	VisualCharacterContext::specificOwner = specificOwner;
	VisualCharacterContext::pWatcher = pWatcher;

	return 0;
}

DEFINE_HOOK(0x703B0B, TechnoClass_VisualCharacter_Normal, 0x5)
{
	if (const HouseClass* const pWatcher = VisualCharacterContext::specificOwner ? VisualCharacterContext::pWatcher : HouseClass::CurrentPlayer)
	{
		const auto pThis = VisualCharacterContext::pThis;
		const auto pTypeExt = TechnoExt::Fetch(pThis)->TypeExtData;
		const auto pOwner = pThis->Owner;
		const auto defaultValue = pTypeExt->DefaultVisualCharacter;

		if (pOwner == pWatcher)
			R->EAX(static_cast<VisualType>(pTypeExt->DefaultVisualCharacterToSelf.Get(defaultValue)));
		else if (pOwner->IsAlliedWith(pWatcher))
			R->EAX(static_cast<VisualType>(pTypeExt->DefaultVisualCharacterToAlly.Get(defaultValue)));
		else
			R->EAX(static_cast<VisualType>(pTypeExt->DefaultVisualCharacterToEnemy.Get(defaultValue)));
	}
	else
	{
		R->EAX(VisualType::Normal);
	}

	return 0;
}

#pragma endregion
