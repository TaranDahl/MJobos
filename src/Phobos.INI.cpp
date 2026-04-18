#include "Phobos.h"

#include <CCINIClass.h>
#include <ScenarioClass.h>
#include <SessionClass.h>
#include <MessageListClass.h>
#include <HouseClass.h>
#include <GameOptionsClass.h>

#include <Utilities/Parser.h>
#include <Utilities/GeneralUtils.h>
#include <Utilities/Patch.h>
#include <Utilities/Macro.h>

const wchar_t* Phobos::UI::CostLabel = L"";
//bool Phobos::UI::DisableEmptySpawnPositions = false;
// 
//bool Phobos::Config::ToolTipDescriptions = true;

DEFINE_HOOK(0x5FACDF, OptionsClass_LoadSettings_LoadPhobosSettings, 0x5)
{
	//const auto phobosSection = "Phobos";

	////Phobos::Config::ToolTipDescriptions = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "ToolTipDescriptions", true);

	//CCINIClass ini_uimd {};
	//ini_uimd.LoadFromFile(GameStrings::UIMD_INI);

	//	ini_uimd.ReadString(GameStrings::ToolTips, "CostLabel", NONE_STR, Phobos::readBuffer);
	//	Phobos::UI::CostLabel = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"$");
	//// LoadingScreen
	//{
	//	Phobos::UI::DisableEmptySpawnPositions =
	//		ini_uimd.ReadBool("LoadingScreen", "DisableEmptySpawnPositions", false);
	//}

	return 0;
}

DEFINE_HOOK(0x52D21F, InitRules_ThingsThatShouldntBeSerailized, 0x6)
{
	//CCINIClass* const pINI_RULESMD = CCINIClass::INI_Rules;

//	Phobos::Config::ArtImageSwap = pINI_RULESMD->ReadBool(GameStrings::General, "ArtImageSwap", false);

	return 0;
}
