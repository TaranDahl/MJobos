#pragma once
#include <Phobos.version.h>
#include <Windows.h>

#include <string>

class CCINIClass;
class AbstractClass;

constexpr auto NONE_STR = "<none>";
constexpr auto NONE_STR2 = "none";
constexpr auto SIDEBAR_SECTION = "Sidebar";
constexpr auto UISETTINGS_SECTION = "UISettings";

class Phobos
{
public:
	static void CmdLineParse(char**, int);

	static void ExeRun();

	//variables
	static HANDLE hInstance;

	static const size_t readLength = 2048;
	static char readBuffer[readLength];
	static wchar_t wideBuffer[readLength];
	static constexpr auto readDelims = ",";

	static const wchar_t* VersionDescription;
	static bool IsLoadingSaveGame;

	static void ApplyOptimizations();

	class UI
	{
	public:
		static const wchar_t* CostLabel;
		//static bool DisableEmptySpawnPositions;
	};

	class Config
	{
	public:
		//static bool ToolTipDescriptions;
	};

	class Misc
	{
	public:
		//static bool CustomGS;
	};

	class Optimizations
	{
	public:
		static bool Applied;
		//static bool DisableRadDamageOnBuildings;
	};
};
