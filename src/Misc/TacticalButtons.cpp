#include "TacticalButtons.h"

#include <SuperClass.h>
#include <AircraftClass.h>
#include <TacticalClass.h>
#include <MouseClass.h>
#include <WWMouseClass.h>
#include <AITriggerTypeClass.h>
#include <JumpjetLocomotionClass.h>
#include <HoverLocomotionClass.h>
#include <InputManagerClass.h>

#include <Ext/UnitType/Body.h>
#include <Ext/AircraftType/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Utilities/TemplateDef.h>
#include <Locomotion/AStar/AStarClass.h>

// TacticalButtonsClass TacticalButtonsClass::Instance;

// Functions
/*
#pragma region PrivateFunctions

int TacticalButtonsClass::CheckMouseOverButtons(const Point2D* pMousePosition)
{
	// TODO New buttons

	if (this->CheckMouseOverBackground(pMousePosition))
		return 0; // Button index 0 : Background

	return -1;
}

bool TacticalButtonsClass::CheckMouseOverBackground(const Point2D* pMousePosition)
{
	// TODO New button backgrounds

	return false;
}

#pragma endregion

#pragma region InlineFunctions

inline bool TacticalButtonsClass::MouseIsOverButtons()
{
	return this->ButtonIndex > 0;
}

inline bool TacticalButtonsClass::MouseIsOverTactical()
{
	return this->ButtonIndex < 0;
}

#pragma endregion

#pragma region CiteFunctions

int TacticalButtonsClass::GetButtonIndex()
{
	return this->ButtonIndex;
}

#pragma endregion

#pragma region GeneralFunctions

void TacticalButtonsClass::SetMouseButtonIndex(const Point2D* pMousePosition)
{
	this->ButtonIndex = this->CheckMouseOverButtons(pMousePosition);

	// TODO New buttons
}

void TacticalButtonsClass::PressDesignatedButton(int triggerIndex)
{
	if (!this->MouseIsOverButtons()) // In buttons background
		return;

	// TODO New buttons
}

#pragma endregion
*/

// Hooks
/*
#pragma region MouseTriggerHooks

DEFINE_HOOK(0x6931A5, ScrollClass_WindowsProcedure_PressLeftMouseButton, 0x6)
{
	enum { SkipGameCode = 0x6931B4 };

	const auto pButtons = &TacticalButtonsClass::Instance;

	if (!pButtons->MouseIsOverTactical())
	{
		pButtons->PressedInButtonsLayer = true;
		pButtons->PressDesignatedButton(0);

		R->Stack(STACK_OFFSET(0x28, 0x8), 0);
		R->EAX(Action::None);
		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x693268, ScrollClass_WindowsProcedure_ReleaseLeftMouseButton, 0x5)
{
	enum { SkipGameCode = 0x693276 };

	const auto pButtons = &TacticalButtonsClass::Instance;

	if (pButtons->PressedInButtonsLayer)
	{
		pButtons->PressedInButtonsLayer = false;
		pButtons->PressDesignatedButton(1);

		R->Stack(STACK_OFFSET(0x28, 0x8), 0);
		R->EAX(Action::None);
		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x69330E, ScrollClass_WindowsProcedure_PressRightMouseButton, 0x6)
{
	enum { SkipGameCode = 0x69334A };

	const auto pButtons = &TacticalButtonsClass::Instance;

	if (!pButtons->MouseIsOverTactical())
	{
		pButtons->PressedInButtonsLayer = true;
		pButtons->PressDesignatedButton(2);

		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x693397, ScrollClass_WindowsProcedure_ReleaseRightMouseButton, 0x6)
{
	enum { SkipGameCode = 0x6933CB };

	const auto pButtons = &TacticalButtonsClass::Instance;

	if (pButtons->PressedInButtonsLayer)
	{
		pButtons->PressedInButtonsLayer = false;
		pButtons->PressDesignatedButton(3);

		return SkipGameCode;
	}

	return 0;
}

#pragma endregion

#pragma region MouseSuspendHooks

DEFINE_HOOK(0x692F85, ScrollClass_MouseUpdate_SkipMouseLongPress, 0x7)
{
	enum { CheckMousePress = 0x692F8E, CheckMouseNoPress = 0x692FDC };

	GET(ScrollClass*, pThis, EBX);

	// 555A: AnyMouseButtonDown
	return (pThis->AnyMouseButtonDown && !TacticalButtonsClass::Instance.PressedInButtonsLayer) ? CheckMousePress : CheckMouseNoPress;
}

DEFINE_HOOK(0x69300B, ScrollClass_MouseUpdate_SkipMouseActionUpdate, 0x6)
{
	enum { SkipGameCode = 0x69301A };

	const auto mousePosition = WWMouseClass::Instance->XY1;
	const auto pButtons = &TacticalButtonsClass::Instance;
	pButtons->SetMouseButtonIndex(&mousePosition);

	if (pButtons->MouseIsOverTactical())
		return 0;

	R->Stack(STACK_OFFSET(0x30, -0x24), 0);
	R->EAX(Action::None);
	return SkipGameCode;
}

#pragma endregion
*/
#pragma region ButtonsDisplayHooks
/*
DEFINE_HOOK(0x6D462C, TacticalClass_Render_DrawBelowTechno, 0x5)
{
	return 0;
}

DEFINE_HOOK(0x6D4941, TacticalClass_Render_DrawButtonCameo, 0x6)
{
	const auto pButtons = &TacticalButtonsClass::Instance;

	// TODO New buttons (The later draw, the higher layer)

	return 0;
}
*/

#pragma endregion

//	Game::SpecialDialog = 0; // 游戏画面
//	Game::SpecialDialog = 1; // 暂停页面
//	Game::SpecialDialog = 2; // 投降页面
//	Game::SpecialDialog = 3; // 退出页面
//	Game::SpecialDialog = 4; // 快捷键设置页面
//	Game::SpecialDialog = 5; // 游戏控制页面
//	Game::SpecialDialog = 6; // 音效控制页面
//	Game::SpecialDialog = 7; // 传送讯息页面
//	Game::SpecialDialog = 8; // 盟友页面
//	Game::SpecialDialog = 9; // 任务简介页面

/*
union VoxelIndexKey
{
	struct MainKey
	{
		uint32_t mainFrame  : 5; // 移动帧号（0-4）
		uint32_t mainFace   : 5; // 车体朝向（5-9）
		uint32_t slopeIndex : 6; // 斜坡索引（10-15）
		uint32_t isSpawnAlt : 1; // SpawnAlt（16）
		uint32_t reserved   : 15;// 空保留位（17-31）
	}
	Main;

	struct TurretKey
	{
		uint32_t turretFace : 5; // 炮塔朝向（0-4）
		uint32_t mainFace   : 5; // 车体朝向（5-9），如果 TurretOffset=0 ，则此段归零，因为从中心点开始画不需要有偏移
		uint32_t slopeIndex : 6; // 斜坡索引（10-15）
		uint32_t turretFrame: 8; // 炮塔帧号（16-23）
		uint32_t turretNum  : 8; // 炮塔编号（24-31）
	}
	Turret;

	struct ShadowKey
	{
		uint32_t offsetY    : 5; // 垂直偏移（0-4）
		uint32_t mainFace   : 5; // 车体朝向（5-9）
		uint32_t slopeIndex : 5; // 斜坡索引（10-14）
		uint32_t reserved   : 16;// 空保留位（15-30）
		uint32_t onGround   : 1; // 位于地面（31）
	}
	Shadow;

	uint32_t Value;
};
*/
