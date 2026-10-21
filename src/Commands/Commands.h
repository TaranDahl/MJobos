#pragma once

#include <CommandClass.h>
#include <StringTable.h>
#include <Phobos.h>

template <typename T>
T* MakeCommand()
{
	T* command = GameCreate<T>();
	CommandClass::Array.AddItem(command);
	return command;
};

enum class ShapeButtonId : int
{
	DistributionMode = 0,
	ManualReload = 1,
	AggressiveStance = 2,
	CeaseFire = 3,
	Reversing = 4
};

class ShapeButtonHelper
{
public:
	using Action = void (*)();

	static constexpr int MaxButtonCount = 25;
	static constexpr int InUseButtonCount = 11;
	static constexpr int UnusedButtonCount = 1;
	static constexpr int OldButtonCount = InUseButtonCount + UnusedButtonCount;
	static constexpr int MaxNewButtonCount = MaxButtonCount - OldButtonCount;

	static bool RegisterButton(ShapeButtonId id, const char* name, const char* tooltip, Action action);
	static bool RegisterButton(ShapeButtonId id, const char* name, const char* tooltip, Action action, int toggleType, bool useFlash);
	static int GetButtonIndex(ShapeButtonId id);
	static bool IsButtonOn(ShapeButtonId id);
	static int GetRegisteredButtonCount();

private:
	struct ButtonEntry
	{
		ShapeButtonId Id;
		const char* Name;
		const char* Tooltip;
		Action Action;
		int ToggleType;
		bool UseFlash;
		bool ConfigureButtonIO;
	};

	static ButtonEntry Buttons[MaxNewButtonCount];
	static int ButtonIndexes[MaxNewButtonCount];
	static int RegisteredButtonCount;

	static int FindSlot(ShapeButtonId id);
	static int FindSlotByButtonIndex(int buttonIndex);
	static bool RegisterButtonInternal(ShapeButtonId id, const char* name, const char* tooltip, Action action, int toggleType, bool useFlash, bool configureButtonIO);

	friend struct ShapeButtonHookAccess;
};

#define CATEGORY_TEAM StringTable::LoadString(GameStrings::TXT_TEAM)
#define CATEGORY_INTERFACE StringTable::LoadString(GameStrings::TXT_INTERFACE)
#define CATEGORY_TAUNT StringTable::LoadString(GameStrings::TXT_TAUNT)
#define CATEGORY_SELECTION StringTable::LoadString(GameStrings::TXT_SELECTION)
#define CATEGORY_CONTROL StringTable::LoadString(GameStrings::TXT_CONTROL)
#define CATEGORY_DEBUG L"Debug"
#define CATEGORY_GUIDEBUG StringTable::LoadString(GameStrings::GUI_DEBUG)
#define CATEGORY_DEVELOPMENT StringTable::LoadString("TXT_DEVELOPMENT")
