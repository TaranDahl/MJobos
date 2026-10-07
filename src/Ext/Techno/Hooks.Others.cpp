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

#pragma region TunnelDist

DEFINE_HOOK(0x74608F, UnitClass_AStarAttempt_SimpleTooFar, 0x5)
{
	enum { GoUnderground = 0x7460F4, GoSurface = 0x746094 };
	GET(const int, simpleDist, EAX);
	return simpleDist >= RulesExt::Global()->TunnelSimpleDistTooFar ? GoUnderground : GoSurface;
}

DEFINE_HOOK(0x7460EC, UnitClass_AStarAttempt_PathingTooFar, 0x5)
{
	GET(const int, pathingDist, EAX);
	R->DL(pathingDist > RulesExt::Global()->TunnelPathingDistTooFar);
	return 0;
}

#pragma endregion
