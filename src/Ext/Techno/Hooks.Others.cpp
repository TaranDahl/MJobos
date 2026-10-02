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

#pragma region RadarDrawing

DEFINE_HOOK(0x47C329, CellClass_GetRadarColor_UnifiedRadarColor, 0x7)
{
	REF_STACK(ColorStruct, rgb1, STACK_OFFSET(0x14, -0x8));
	REF_STACK(ColorStruct, rgb2, STACK_OFFSET(0x14, -0xC));
	GET(CellClass*, pThis, ESI);

	const auto pRulesExt = RulesExt::Global();

	if (!pRulesExt->UnifiedRadarColor)
		return 0;

	if (pThis->Tile_Is_Cliff())
		rgb1 = pRulesExt->UnifiedRadarColor_Cliff;
	else if (pThis->Tile_Is_Water())
		rgb1 = pRulesExt->UnifiedRadarColor_Water;
	else
		rgb1 = pRulesExt->UnifiedRadarColor_Land;

	rgb2 = rgb1;

	return 0;
}

DEFINE_HOOK(0x655E58, RadarClass_ProcessPoint_DrawOccupiable, 0x6)
{
	GET(TechnoClass*, pTechno, EBP);

	const auto pBuilding = abstract_cast<BuildingClass*>(pTechno);
	const bool noDraw = pTechno->Owner->Type->MultiplayPassive
		&& (!RulesExt::Global()->UnifiedRadarColor
			|| !pBuilding
			|| !pBuilding->Type->CanBeOccupied);

	R->CL(noDraw);
	return R->Origin() + 0x6;
}

#pragma endregion

#pragma region UnifiedTechnoColor

DEFINE_HOOK(0x655F80, RadarClass_ProcessPoint_UnifiedRadarColor, 0x6)
{
	enum { SkipGameCode = 0x655FEB };

	GET_STACK(HouseClass*, pOwner, STACK_OFFSET(0x40, 0x4));

	if (!Phobos::Config::UnifiedTechnoColor)
		return 0;

	const auto pRulesExt = RulesExt::Global();

	if (!pRulesExt->UnifiedRadarColor)
		return 0;

	int colorCode = 0;

	if (pOwner->Type->MultiplayPassive)
		colorCode = Drawing::RGB_To_Int(pRulesExt->UnifiedRadarColor_Neutral);
	else if (pOwner->IsControlledByCurrentPlayer())
		colorCode = Drawing::RGB_To_Int(pRulesExt->UnifiedRadarColor_Self);
	else if (HouseClass::CurrentPlayer->IsAlliedWith(pOwner))
		colorCode = Drawing::RGB_To_Int(pRulesExt->UnifiedRadarColor_Ally);
	else
		colorCode = Drawing::RGB_To_Int(pRulesExt->UnifiedRadarColor_Enemy);

	R->EBX(colorCode);
	return SkipGameCode;
}

DEFINE_HOOK(0x705D88, TechnoClass_GetRemapColour_UnifiedColor, 0x8)
{
	enum { SkipGameCode = 0x705DF1 };

	GET(TechnoClass*, pThis, ESI);
	GET(DynamicVectorClass<ColorScheme*>*, pPalette, EAX);

	if (!Phobos::Config::UnifiedTechnoColor)
		return 0;

	auto getOwner = [pThis]()
	{
		if (pThis->IsClearlyVisibleTo(HouseClass::CurrentPlayer))
			return pThis->Owner;

		if (const auto pDisguiseHouse = pThis->GetDisguiseHouse(true))
			return pDisguiseHouse;

		return pThis->Owner;
	};
	const auto pOwner = getOwner();

	auto getSchemeIdx = [pOwner]()
	{
		const auto pRulesExt = RulesExt::Global();

		if (pOwner->Type->MultiplayPassive)
			return pRulesExt->UnifiedTechnoColor_Neutral == -1 ? ColorScheme::FindIndex("LightGrey") : pRulesExt->UnifiedTechnoColor_Neutral;
		else if (pOwner->IsControlledByCurrentPlayer())
			return pRulesExt->UnifiedTechnoColor_Self == -1 ? ColorScheme::FindIndex("Green") : pRulesExt->UnifiedTechnoColor_Self;
		else if (HouseClass::CurrentPlayer->IsAlliedWith(pOwner))
			return pRulesExt->UnifiedTechnoColor_Ally == -1 ? ColorScheme::FindIndex("Gold") : pRulesExt->UnifiedTechnoColor_Ally;

		return pRulesExt->UnifiedTechnoColor_Enemy == -1 ? ColorScheme::FindIndex("Red") : pRulesExt->UnifiedTechnoColor_Enemy;
	};
	const int unifiedColorScheme = getSchemeIdx();
	const int colorSchemeIdx = unifiedColorScheme != -1 ? unifiedColorScheme : pOwner->ColorSchemeIndex;

	R->ECX(pPalette ? pPalette->Items[colorSchemeIdx] : ColorScheme::Array.Items[colorSchemeIdx]);
	return SkipGameCode;
}

#pragma endregion

