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

#pragma region JumpjetSpeedType

DEFINE_HOOK(0x54B36A, JumpjetLocomotionClass_MoveTo_JumpjetSpeedType, 0x5)
{
	GET(ILocomotion* const, iloco, ESI);
	REF_STACK(SpeedType, speedType, STACK_OFFSET(0x5C, -0x54));

	__assume(iloco != nullptr);
	const auto pLoco = static_cast<JumpjetLocomotionClass*>(iloco);
	const auto pTypeExt = TechnoExt::Fetch(pLoco->LinkedTo)->TypeExtData;
	speedType = static_cast<SpeedType>(pTypeExt->JumpjetSpeedType.Get());

	return 0;
}

#pragma endregion
