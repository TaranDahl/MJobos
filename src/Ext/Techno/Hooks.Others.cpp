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

#pragma region FixRepairDistance

DEFINE_PATCH(0x44C70B, 0xC8);
DEFINE_JUMP(LJMP, 0x44C75E, 0x44C793);

#pragma endregion

#pragma region PlanWaypoint

DEFINE_HOOK(0x63745D, UnknownClass_PlanWaypoint_ContinuePlanningOnEnter, 0x6)
{
	enum { SkipDeselect = 0x637468 };

	GET(const int, planResult, ESI);

	return (!planResult && !RulesExt::Global()->StopPlanningOnEnter) ? SkipDeselect : 0;
}

DEFINE_HOOK(0x637479, UnknownClass_PlanWaypoint_DisableMessage, 0x5)
{
	enum { SkipMessage = 0x637524 };
	return (!RulesExt::Global()->StopPlanningOnEnter) ? SkipMessage : 0;
}

DEFINE_HOOK(0x638D73, UnknownClass_CheckLastWaypoint_ContinuePlanningWaypoint, 0x5)
{
	enum { SkipDeselect = 0x638D8D, Deselect = 0x638D82 };

	GET(const Action, action, EAX);

	if (!RulesExt::Global()->StopPlanningOnEnter)
		return SkipDeselect;
	else if (action == Action::Select || action == Action::ToggleSelect || action == Action::Capture)
		return Deselect;

	return SkipDeselect;
}

#pragma endregion

#pragma region CrushBuildingOnAnyCell

namespace CrushBuildingOnAnyCell
{
	CellClass* pCell;
}

DEFINE_HOOK(0x741733, UnitClass_CrushCell_SetContext, 0x6)
{
	GET(CellClass*, pCell, ESI);

	CrushBuildingOnAnyCell::pCell = pCell;

	return 0;
}

DEFINE_HOOK(0x741925, UnitClass_CrushCell_CrushBuilding, 0x5)
{
	GET(UnitClass*, pThis, EDI);

	if (RulesExt::Global()->CrushBuildingOnAnyCell)
	{
		if (auto const pBuilding = CrushBuildingOnAnyCell::pCell->GetBuilding())
		{
			if (pBuilding->IsCrushable(pThis))
			{
				VocClass::PlayAt(pBuilding->Type->CrushSound, pThis->Location, 0);
				pBuilding->Destroy();
				pBuilding->RegisterDestruction(pThis);
				pBuilding->Mark(MarkType::Up);
				// pBuilding->Limbo(); // Vanilla do this. May be not necessary?
				pBuilding->UnInit();

				R->AL(true);
			}
		}
	}

	return 0;
}

#pragma endregion

#pragma region UpdateReload

DEFINE_HOOK(0x51BDCF, InfantryClass_Update_Reload, 0x7)
{
	enum { SkipGameCode = 0x51BDD6 };

	GET(InfantryClass*, pThis, ESI);

	R->EAX(pThis->InWhichLayer());

	if (RulesExt::Global()->InTransportInfantryAmmoFix && AresHelper::CanUseAres && pThis->InLimbo && !TechnoTypeExt::Fetch(pThis->Type)->ReloadInTransport)
		pThis->Reload();

	return SkipGameCode;
}

#pragma endregion

#pragma region IgnoreByMouse

static inline bool ShouldIgnoreByMouse(ObjectClass* pObject)
{
	const auto pType = pObject->GetType();

	if (!pType)
		return true;

	if (const auto pTechno = abstract_cast<TechnoClass*, true>(pObject))
	{
		const auto pTypeExt = TechnoExt::Fetch(pTechno)->TypeExtData;
		const auto pOwner = pTechno->Owner;
		const auto defaultValue = pTypeExt->IgnoredByMouse;

		if (pOwner == HouseClass::CurrentPlayer)
			return pTypeExt->IgnoredByMouse_ToSelf.Get(defaultValue);
		else if (pOwner->IsAlliedWith(HouseClass::CurrentPlayer))
			return pTypeExt->IgnoredByMouse_ToAlly.Get(defaultValue);

		return pTypeExt->IgnoredByMouse_ToEnemy.Get(defaultValue);
	}

	const auto absType = pObject->WhatAmI();

	if (absType == OverlayClass::AbsID)
	{
		const auto pOverlay = static_cast<OverlayClass*>(pObject);
		const auto pTypeExt = OverlayTypeExt::Fetch(pOverlay->Type);
		return pTypeExt->IgnoredByMouse.Get();
	}
	else if (absType == TerrainClass::AbsID)
	{
		const auto pTerrain = static_cast<TerrainClass*>(pObject);
		const auto pTypeExt = TerrainTypeExt::Fetch(pTerrain->Type);
		return pTypeExt->IgnoredByMouse.Get();
	}

	return false;
}

DEFINE_HOOK(0x6DA3D2, TacticalClass_GetObjectOnCrd_IgnoredByMouse1, 0x8)
{
	enum { Ignore = 0x6DA491, DontIgnore = 0x6DA3DA };

	GET(ObjectClass* const, pObject, ESI);

	return !pObject || ShouldIgnoreByMouse(pObject) ? Ignore : DontIgnore;
}

DEFINE_HOOK(0x6DA4FB, TacticalClass_GetObjectOnCrd_IgnoredByMouse2, 0x6)
{
	enum { SkipGameCode = 0x6DA501 };

	GET(CellClass* const, pCell, EAX);

	ObjectClass* pFoundObject = nullptr;

	for (auto pOccupier = pCell->FirstObject; pOccupier; pOccupier = pOccupier->NextObject)
	{
		if (ShouldIgnoreByMouse(pOccupier))
			continue;
		pFoundObject = pOccupier;
		break;
	}

	R->EAX(pFoundObject);
	return SkipGameCode;
}

#pragma endregion

#pragma region HealingWeaponFix

// Skip the hardcode of healing weapon auto target range.
DEFINE_JUMP(LJMP, 0x6F9024, 0x6F9042);

DEFINE_HOOK(0x6FA9D8, TechnoClass_Update_FixRepairWeapon, 0x6)
{
	enum { SkipGameCode = 0x6FAA6F };

	GET(TechnoClass*, pThis, ESI);

	if (pThis->Target && pThis->CombatDamage(-1) < 0)
	{
		if ((SessionClass::IsCampaign() ? pThis->Owner->IsControlledByCurrentPlayer() : pThis->Owner->IsHumanPlayer) && !pThis->Owner->IsAlliedWith(pThis->Target))
			pThis->SetTarget(nullptr);
		else if (pThis->CurrentMission == Mission::Guard && !pThis->IsCloseEnoughToAttack(pThis->Target))
			pThis->SetTarget(nullptr);
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x707ED0, TechnoClass_GetGuardRange_FixForIFV, 0x6)
{
	enum { SkipGameCode = 0x707F08 };

	GET(TechnoClass*, pThis, ESI);

	auto pType = pThis->GetTechnoType();

	if (!pType->HasMultipleTurrets() || pType->IsGattling)
		return 0;

	R->EAX(pThis->GetWeaponRange(pThis->CurrentWeaponNumber));
	return SkipGameCode;
}

#pragma endregion

#pragma region SharedControl

DEFINE_HOOK(0x50B716, HouseClass_IsCurrentPlayer_SharedControl, 0x6)
{
	GET(HouseClass*, pThis, ECX);
	R->EAX(RulesExt::Global()->AllyShareControl ? pThis->IsAlliedWith(HouseClass::CurrentPlayer) : pThis == HouseClass::CurrentPlayer);
	return 0x50B723;
}

#pragma endregion

#pragma region AIAdjacentMax

DEFINE_HOOK_AGAIN(0x42EB6A, BaseClass_GetBaseNodeIndex_AIAdjacentMax, 0x8);
DEFINE_HOOK(0x42EBA2, BaseClass_GetBaseNodeIndex_AIAdjacentMax, 0x8)
{
	GET(BaseClass*, pThis, ESI);
	GET(const int, nodeIdx, EDI);

	bool isValid = reinterpret_cast<bool(__thiscall*)(BaseClass*, int)>(0x42E780)(pThis, nodeIdx);
	const int rangeLimit = SessionClass::Instance.IsCampaign()
		? RulesExt::Global()->AIAdjacentMax_Campaign.Get(RulesExt::Global()->AIAdjacentMax)
		: RulesExt::Global()->AIAdjacentMax;

	if (rangeLimit >= 0 && isValid)
	{
		const auto node = pThis->BaseNodes[nodeIdx];
		const auto pOwner = pThis->Owner;
		const auto pBuildingType = BuildingTypeClass::Array[node.BuildingTypeIndex];
		const CellStruct offset
		{
			static_cast<short>(pBuildingType->GetFoundationWidth() / 2),
			static_cast<short>(pBuildingType->GetFoundationHeight(false) / 2)
		};
		const auto center = node.MapCoords + offset;
		const auto cellList = GeneralUtils::AdjacentCellsInRange(rangeLimit);
		bool hasAdjacent = false;

		for (const auto& cell : cellList)
		{
			const auto pBuilding = MapClass::Instance.GetCellAt(cell + center)->GetBuilding();

			if (!pBuilding || pBuilding->Owner != pOwner)
				continue;

			const auto pType = pBuilding->Type;
			const auto baseNormalDefault = (!pType->UndeploysInto || !pType->ResourceGatherer) && !pBuilding->IsStrange();

			if (BuildingTypeExt::Fetch(pType)->AIBaseNormal.Get(baseNormalDefault))
			{
				hasAdjacent = true;
				break;
			}
		}

		isValid = hasAdjacent;
	}

	R->AL(isValid);
	return R->Origin() + 0x8;
}

#pragma endregion

#pragma region LoadGameTips

static inline void LoadTips(INI_EX exINI, const char* pSection, DynamicVectorClass<CSFText>& dest)
{
    char tempBuffer[32];
    for (size_t i = 0; i < 1024; ++i)
    {
        Valueable<CSFText> tip;
        _snprintf_s(tempBuffer, sizeof(tempBuffer), "Tip%d", i);
        tip.Read(exINI, pSection, tempBuffer);

        if (!tip.Get())
            return;

        dest.AddUnique(tip);
    }
}

DEFINE_HOOK(0x68758D, INIClass_ReadScenario_AfterLoadProgressMgrDraw, 0x5)
{
	// Get the text to draw.
	GET(CCINIClass*, pMapINI, EBP);

	// The RulesExt has not been read yet. Thus we manually read the ini here.
	const auto pRulesINI = CCINIClass::INI_Rules;
	auto pText = L"";
	DynamicVectorClass<CSFText> availableTexts;
	const bool useMapTipsOnly = pMapINI->ReadBool(GameStrings::Basic, "UseMapTipsOnly", false);
	INI_EX exMapINI(pMapINI);
	LoadTips(exMapINI, GameStrings::Basic, availableTexts);

	if (!useMapTipsOnly)
	{
		INI_EX exINI(pRulesINI);
		LoadTips(exINI, GameStrings::General, availableTexts);

		if (SessionClass::Instance.GameMode != GameMode::Campaign)
		{
			const int countryIdx = NodeNameType::Array[0]->Country;

			if (countryIdx >= 0 && countryIdx < HouseTypeClass::Array.Count)
			{
				const auto pCountryName = HouseTypeClass::Array.GetItem(countryIdx)->ID;
				LoadTips(exINI, pCountryName, availableTexts);
			}
		}
	}

	if (availableTexts.Count <= 0)
		return 0;

	srand(static_cast<unsigned int>(time(NULL)));
	pText = availableTexts.GetItem(rand() % availableTexts.Count).Text;

	// Calculate the rect.
	constexpr int assumedTextHeight = 15;
	constexpr int gapToBorder = 8;
	constexpr int gapToAnotherLine = 5;
	const int tipBarWidth = pRulesINI->ReadInteger(GameStrings::General, "TipBarWidth", 700);
	const bool tipBarTwoLines = pRulesINI->ReadBool(GameStrings::General, "TipBarTwoLines", false);
	const int width = tipBarWidth;
	const int height = tipBarTwoLines ? (assumedTextHeight * 2 + gapToBorder * 2 + gapToAnotherLine) : (assumedTextHeight + gapToBorder * 2);
	const auto barRect = LoadProgressManager::Instance->LoadBarSHPRect;
	const int x = barRect.X + (barRect.Width - width) / 2;
	const int y = barRect.Y + barRect.Height - height - 5;
	auto rect = RectangleStruct { x, y, width, height };

	// Draw background.
	const auto pSurface = LoadProgressManager::Instance->ProgressSurface;
	pSurface->FillRect(&rect, COLOR_BLACK);
	pSurface->DrawRect(&rect, COLOR_WHITE);

	// Draw the text.
	constexpr TextPrintType printType = TextPrintType::Center | TextPrintType::Point8;

	if (tipBarTwoLines)
	{
		// Split text at first newline.
		// Only support one or two lines.
		const wchar_t* pFirstPart = pText;
		const wchar_t* pSecondPart = nullptr;
		std::unique_ptr<wchar_t[]> pFirstBuffer;

		if (const wchar_t* pNewline = wcschr(pText, L'\n'))
		{
			// Create temporary buffer for first part
			size_t firstLen = pNewline - pText;
			pFirstBuffer = std::make_unique<wchar_t[]>(firstLen + 1);
			wcsncpy(pFirstBuffer.get(), pText, firstLen);
			pFirstBuffer[firstLen] = L'\0';

			pFirstPart = pFirstBuffer.get();
			pSecondPart = pNewline + 1;
		}

		if (pSecondPart)
		{
			auto location = Point2D { rect.Width / 2, gapToBorder };
			pSurface->DrawTextA(pFirstPart, &rect, &location, COLOR_WHITE, 0, printType);
			location = Point2D { rect.Width / 2, gapToBorder + assumedTextHeight + gapToAnotherLine };
			pSurface->DrawTextA(pSecondPart, &rect, &location, COLOR_WHITE, 0, printType);
		}
		else
		{
			const int gapToBorderOneLine = (height - assumedTextHeight) / 2;
			auto location = Point2D { rect.Width / 2, gapToBorderOneLine };
			pSurface->DrawTextA(pText, &rect, &location, COLOR_WHITE, 0, printType);
		}
	}
	else
	{
		auto location = Point2D { rect.Width / 2, gapToBorder };
		pSurface->DrawTextA(pText, &rect, &location, COLOR_WHITE, 0, printType);
	}

	return 0;
}

#pragma endregion

#pragma region AIProtectBase

DEFINE_HOOK(0x70821F, TechnoClass_BaseIsAttacked_Ignore1, 0x6)
{
	enum { CheckDefend = 0x70822B, SkipDefend = 0x7083BC };
	GET(TeamClass*, pTeam, EAX);
	GET(bool, isBaseDefense, ECX);
	GET(FootClass*, pThis, ESI);

	if (isBaseDefense)
		return CheckDefend;

	if (TechnoTypeExt::Fetch(pThis->GetTechnoType())->AIDefendBase_Ignore)
		return SkipDefend;

	return pTeam ? SkipDefend : CheckDefend;
}

DEFINE_HOOK(0x708455, TechnoClass_BaseIsAttacked_Ignore2, 0x6)
{
	enum { CheckDefend = 0x708461, SkipDefend = 0x708622 };
	GET(TeamClass*, pTeam, EAX);
	GET(bool, isBaseDefense, ECX);
	GET(FootClass*, pThis, ESI);

	if (isBaseDefense)
		return CheckDefend;

	if (TechnoTypeExt::Fetch(pThis->GetTechnoType())->AIDefendBase_Ignore)
		return SkipDefend;

	return pTeam ? SkipDefend : CheckDefend;
}

#pragma endregion

#pragma region NoAutoFire

DEFINE_HOOK(0x6F8E44, TechnoClass_SelectAutoTarget_NoAutoFire, 0x7)
{
	GET(TechnoClass*, pThis, ESI);
	return TechnoTypeExt::Fetch(pThis->GetTechnoType())->NoAutoFire_AI && !pThis->Owner->IsControlledByHuman() ? 0x6F8E38 : 0;
}

#pragma endregion

#pragma region KeepAnimOnLimbo

DEFINE_HOOK(0x422C70, AnimClass_DrawIfVisible_DontDrawIfOwnerInLimbo, 0x6)
{
	if (!RulesExt::Global()->KeepAnimOnLimbo)
		return 0;

	GET(AnimClass*, pThis, ECX);

	R->EAX(pThis->LoopDelay || pThis->OwnerObject && (pThis->OwnerObject->InLimbo || pThis->OwnerObject->VisualCharacter(true, HouseClass::CurrentPlayer) == VisualType::Hidden));
	return R->Origin() + 0x6;
}

DEFINE_HOOK(0x425174, AnimClass_PointerExpired_KeepAnimOnLimbo, 0x6)
{
	if (!RulesExt::Global()->KeepAnimOnLimbo)
		return 0;

	GET_STACK(bool, bRemoved, STACK_OFFSET(0xC, 0x8));

	return bRemoved ? 0 : 0x4251A3;
}

#pragma endregion
