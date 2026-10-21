#include "Commands.h"

#include "ObjectInfo.h"
#include "NextIdleHarvester.h"
#include "QuickSave.h"
#include "DamageDisplay.h"
#include "FrameByFrame.h"
#include "FrameStep.h"
#include "ToggleDigitalDisplay.h"
#include "ToggleDesignatorRange.h"
#include "SaveVariablesToFile.h"
#include "SelectCaptured.h"
#include "DistributionMode.h"
#include "ToggleSWSidebar.h"
#include "FireTacticalSW.h"
#include "ToggleMessageList.h"
#include "DeselectObject.h"
#include "DeselectObject5.h"

#include <CCINIClass.h>
#include <ShapeButtonClass.h>

#include <Ext/Sidebar/SWSidebar/SWSidebarClass.h>
#include <Misc/MessageColumn.h>

DEFINE_HOOK(0x533066, CommandClassCallback_Register, 0x6)
{
	// Load it after Ares'
	// Feature layers register shape buttons here, before TabClass initializes.

	MakeCommand<NextIdleHarvesterCommandClass>();
	MakeCommand<QuickSaveCommandClass>();
	MakeCommand<ToggleDigitalDisplayCommandClass>();
	MakeCommand<ToggleDesignatorRangeCommandClass>();
	MakeCommand<ToggleMessageListCommandClass>();
	MakeCommand<ToggleSWSidebar>();
	MakeCommand<DeselectObjectCommandClass>();
	MakeCommand<DeselectObject5CommandClass>();

	if (Phobos::Config::SelectCapturedCommand)
		MakeCommand<SelectCapturedCommandClass>();

	if (Phobos::Config::SuperWeaponSidebarCommands)
	{
		SWSidebarClass::Commands[0] = MakeCommand<FireTacticalSWCommandClass<0>>();
		SWSidebarClass::Commands[1] = MakeCommand<FireTacticalSWCommandClass<1>>();
		SWSidebarClass::Commands[2] = MakeCommand<FireTacticalSWCommandClass<2>>();
		SWSidebarClass::Commands[3] = MakeCommand<FireTacticalSWCommandClass<3>>();
		SWSidebarClass::Commands[4] = MakeCommand<FireTacticalSWCommandClass<4>>();
		SWSidebarClass::Commands[5] = MakeCommand<FireTacticalSWCommandClass<5>>();
		SWSidebarClass::Commands[6] = MakeCommand<FireTacticalSWCommandClass<6>>();
		SWSidebarClass::Commands[7] = MakeCommand<FireTacticalSWCommandClass<7>>();
		SWSidebarClass::Commands[8] = MakeCommand<FireTacticalSWCommandClass<8>>();
		SWSidebarClass::Commands[9] = MakeCommand<FireTacticalSWCommandClass<9>>();
	}

	if (Phobos::Config::AllowSwitchNoMoveCommand)
		MakeCommand<SwitchNoMoveCommandClass>();

	if (Phobos::Config::AllowDistributionCommand)
	{
		if (Phobos::Config::AllowDistributionCommand_SpreadMode)
			MakeCommand<DistributionModeSpreadCommandClass>();

		if (Phobos::Config::AllowDistributionCommand_FilterMode)
			MakeCommand<DistributionModeFilterCommandClass>();

		MakeCommand<DistributionModeHoldDownCommandClass>();

		ShapeButtonHelper::RegisterButton(
			ShapeButtonId::DistributionMode,
			"DistributionMode",
			"Tip:DistributionMode",
			&DistributionModeHoldDownCommandClass::ToggleExecute,
			1,
			true);
	}

	if (Phobos::Config::DevelopmentCommands)
	{
		MakeCommand<DamageDisplayCommandClass>();
		MakeCommand<SaveVariablesToFileCommandClass>();
		MakeCommand<ObjectInfoCommandClass>();
		MakeCommand<FrameByFrameCommandClass>();
		MakeCommand<FrameStepCommandClass<1>>(); // Single step in
		MakeCommand<FrameStepCommandClass<5>>(); // Speed 1
		MakeCommand<FrameStepCommandClass<10>>(); // Speed 2
		MakeCommand<FrameStepCommandClass<15>>(); // Speed 3
		MakeCommand<FrameStepCommandClass<30>>(); // Speed 4
		MakeCommand<FrameStepCommandClass<60>>(); // Speed 5
	}

	return 0;
}

static void MouseWheelDownCommand()
{
	if (DistributionModeHoldDownCommandClass::Enabled && Phobos::Config::AllowDistributionCommand_SpreadModeScroll)
		DistributionModeHoldDownCommandClass::DistributionSpreadModeReduce();

	if (MessageColumnClass::Instance.IsHovering())
		MessageColumnClass::Instance.ScrollDown();
}

static void MouseWheelUpCommand()
{
	if (DistributionModeHoldDownCommandClass::Enabled && Phobos::Config::AllowDistributionCommand_SpreadModeScroll)
		DistributionModeHoldDownCommandClass::DistributionSpreadModeExpand();

	if (MessageColumnClass::Instance.IsHovering())
		MessageColumnClass::Instance.ScrollUp();
}

DEFINE_HOOK(0x777998, Game_WndProc_ScrollMouseWheel, 0x6)
{
	GET(const WPARAM, WParam, ECX);

	if (WParam & 0x80000000u)
		MouseWheelDownCommand();
	else
		MouseWheelUpCommand();

	return 0;
}

static inline bool CheckSkipScrollSidebar()
{
	return DistributionModeHoldDownCommandClass::Enabled
		|| MessageColumnClass::Instance.IsHovering();
}

DEFINE_HOOK(0x533F50, Game_ScrollSidebar_Skip, 0x5)
{
	enum { SkipScrollSidebar = 0x533FC3 };
	return CheckSkipScrollSidebar() ? SkipScrollSidebar : 0;
}

#pragma region ShapeButton

ShapeButtonHelper::ButtonEntry ShapeButtonHelper::Buttons[ShapeButtonHelper::MaxNewButtonCount] {};
int ShapeButtonHelper::ButtonIndexes[ShapeButtonHelper::MaxNewButtonCount] {};
int ShapeButtonHelper::RegisteredButtonCount = 0;

bool ShapeButtonHelper::RegisterButton(ShapeButtonId id, const char* name, const char* tooltip, Action action)
{
	return RegisterButtonInternal(id, name, tooltip, action, 0, false, false);
}

bool ShapeButtonHelper::RegisterButton(ShapeButtonId id, const char* name, const char* tooltip, Action action, int toggleType, bool useFlash)
{
	return RegisterButtonInternal(id, name, tooltip, action, toggleType, useFlash, true);
}

bool ShapeButtonHelper::RegisterButtonInternal(ShapeButtonId id, const char* name, const char* tooltip, Action action, int toggleType, bool useFlash, bool configureButtonIO)
{
	const int idValue = static_cast<int>(id);

	if (idValue < 0 || idValue >= MaxNewButtonCount || !name || !*name || !tooltip || !*tooltip || !action)
		return false;

	if (FindSlot(id) >= 0)
		return false;

	for (int i = 0; i < RegisteredButtonCount; ++i)
	{
		if (_strcmpi(name, Buttons[i].Name) == 0)
			return false;
	}

	if (RegisteredButtonCount >= MaxNewButtonCount)
		return false;

	int slot = 0;

	while (slot < RegisteredButtonCount && static_cast<int>(Buttons[slot].Id) < idValue)
		++slot;

	for (int i = RegisteredButtonCount; i > slot; --i)
	{
		Buttons[i] = Buttons[i - 1];
		ButtonIndexes[i] = ButtonIndexes[i - 1];
	}

	Buttons[slot] = { id, name, tooltip, action, toggleType, useFlash, configureButtonIO };
	ButtonIndexes[slot] = -1;
	++RegisteredButtonCount;
	return true;
}

int ShapeButtonHelper::GetButtonIndex(ShapeButtonId id)
{
	const int slot = FindSlot(id);
	return slot >= 0 ? ButtonIndexes[slot] : -1;
}

bool ShapeButtonHelper::IsButtonOn(ShapeButtonId id)
{
	const int buttonIndex = GetButtonIndex(id);

	if (buttonIndex < 0)
		return false;

	const auto pButton = ShapeButtonClass::GetButton(buttonIndex);
	return pButton && pButton->IsOn;
}

int ShapeButtonHelper::GetRegisteredButtonCount()
{
	return RegisteredButtonCount;
}

int ShapeButtonHelper::FindSlot(ShapeButtonId id)
{
	for (int i = 0; i < RegisteredButtonCount; ++i)
	{
		if (Buttons[i].Id == id)
			return i;
	}

	return -1;
}

int ShapeButtonHelper::FindSlotByButtonIndex(int buttonIndex)
{
	if (buttonIndex < OldButtonCount)
		return -1;

	for (int i = 0; i < RegisteredButtonCount; ++i)
	{
		if (ButtonIndexes[i] == buttonIndex)
			return i;
	}

	return -1;
}

struct ShapeButtonHookAccess
{
	static int FindCompactIndex(const char* name)
	{
		if (!name)
			return -1;

		for (int i = 0; i < ShapeButtonHelper::RegisteredButtonCount; ++i)
		{
			if (_strcmpi(name, ShapeButtonHelper::Buttons[i].Name) == 0)
				return i;
		}

		return -1;
	}

	static void ResolveButtonIndexes()
	{
		for (int i = 0; i < ShapeButtonHelper::RegisteredButtonCount; ++i)
			ShapeButtonHelper::ButtonIndexes[i] = ShapeButtonClass::FindIndex(ShapeButtonHelper::Buttons[i].Name);
	}

	static void InvokeButtonAction(int buttonIndex)
	{
		const int slot = ShapeButtonHelper::FindSlotByButtonIndex(buttonIndex);

		if (slot >= 0)
			ShapeButtonHelper::Buttons[slot].Action();
	}

	static void ApplyButtonOptions()
	{
		for (int i = 0; i < ShapeButtonHelper::RegisteredButtonCount; ++i)
		{
			if (!ShapeButtonHelper::Buttons[i].ConfigureButtonIO)
				continue;

			const int buttonIndex = ShapeButtonHelper::ButtonIndexes[i];

			if (buttonIndex < 0)
				continue;

			if (const auto pButton = ShapeButtonClass::GetButton(buttonIndex))
			{
				pButton->ToggleType = ShapeButtonHelper::Buttons[i].ToggleType;
				pButton->UseFlash = ShapeButtonHelper::Buttons[i].UseFlash;
			}
		}
	}

	static void ApplyButtonToolTips()
	{
		for (int i = 0; i < ShapeButtonHelper::RegisteredButtonCount; ++i)
		{
			const int buttonIndex = ShapeButtonHelper::ButtonIndexes[i];

			if (buttonIndex < 0)
				continue;

			if (const auto pButton = ShapeButtonClass::GetButton(buttonIndex))
				ShapeButtonClass::SetToolTip(pButton, ShapeButtonHelper::Buttons[i].Tooltip);
		}
	}
};

DEFINE_HOOK(0x6CFD08, ShapeButtonClass_FindIndex_FindNewButton, 0x5)
{
	enum { SetButtonIndex = 0x6CFD0D };

	GET(const char*, name, ECX);

	const int compactIndex = ShapeButtonHookAccess::FindCompactIndex(name);

	if (compactIndex >= 0)
	{
		R->EAX(compactIndex + ShapeButtonHelper::OldButtonCount);
		return SetButtonIndex;
	}

	return 0;
}

DEFINE_HOOK(0x6D0233, TabClass_Init_InitNewButtonIndex, 0x6)
{
	ShapeButtonHookAccess::ResolveButtonIndexes();

	return 0;
}

DEFINE_HOOK(0x6D0827, TabClass_Update_UpdateNewButton, 0x6)
{
	GET(const int, index, EAX);

	ShapeButtonHookAccess::InvokeButtonAction(index);

	return 0;
}

DEFINE_HOOK(0x6D10DF, TabClass_InitButtonIO_InitNewHoldDownButton, 0x6)
{
	ShapeButtonHookAccess::ApplyButtonOptions();

	return 0;
}

DEFINE_HOOK(0x6D14DD, TabClass_InitToolTip_InitNewButtonToolTip, 0x5)
{
	ShapeButtonHookAccess::ApplyButtonToolTips();

	return 0;
}

#pragma endregion
