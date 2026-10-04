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

#pragma region Decloak

DEFINE_HOOK(0x6FBC74, TechnoClass_UpdateCloak_LowHealth, 0x6)
{
	return RulesExt::Global()->Decloak_OnCloakingWithLowHealth ? 0 : R->Origin() + 0xC;
}

DEFINE_HOOK(0x74192E, UnitClass_CrushCell_Decloak, 0x5)
{
	return RulesExt::Global()->Decloak_OnCrushing ? 0 : R->Origin() + 0xB;
}

DEFINE_HOOK_AGAIN(0x75BBA1, SomeLocomotionClass_WhileMoving_DecloakBlockage, 0x6); // Walk
DEFINE_HOOK_AGAIN(0x6A3A7A, SomeLocomotionClass_WhileMoving_DecloakBlockage, 0x6); // Ship
DEFINE_HOOK_AGAIN(0x6A2FA0, SomeLocomotionClass_WhileMoving_DecloakBlockage, 0x6); // Ship
DEFINE_HOOK_AGAIN(0x6A1499, SomeLocomotionClass_WhileMoving_DecloakBlockage, 0x6); // Ship
DEFINE_HOOK_AGAIN(0x5B0F33, SomeLocomotionClass_WhileMoving_DecloakBlockage, 0x6); // Mech
DEFINE_HOOK_AGAIN(0x51558A, SomeLocomotionClass_WhileMoving_DecloakBlockage, 0x6); // Hover
DEFINE_HOOK_AGAIN(0x4B444E, SomeLocomotionClass_WhileMoving_DecloakBlockage, 0x6); // Drive
DEFINE_HOOK_AGAIN(0x4B3951, SomeLocomotionClass_WhileMoving_DecloakBlockage, 0x6); // Drive
DEFINE_HOOK(0x4B1E56, SomeLocomotionClass_WhileMoving_DecloakBlockage, 0x6) // Drive
{
	return RulesExt::Global()->Decloak_OnBlockingMovement ? 0 : R->Origin() + 0x12;
}

#pragma endregion
