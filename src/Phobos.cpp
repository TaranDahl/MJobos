#include "Phobos.h"

#include <Drawing.h>
#include <SessionClass.h>
#include <Unsorted.h>

#include <Utilities/Debug.h>
#include <Utilities/Patch.h>
#include <Utilities/Macro.h>
#include "Utilities/AresHelper.h"
#include "Utilities/Parser.h"

HANDLE Phobos::hInstance = 0;

char Phobos::readBuffer[Phobos::readLength];
wchar_t Phobos::wideBuffer[Phobos::readLength];

bool Phobos::IsLoadingSaveGame = false;

bool Phobos::Optimizations::Applied = false;

void Phobos::CmdLineParse(char** ppArgs, int nNumArgs)
{
	//bool foundInheritance = false;

	Parser<bool> boolParser { };

	// > 1 because the exe path itself counts as an argument, too!
	for (int i = 1; i < nNumArgs; i++)
	{
		const char* pArg = ppArgs[i];
		std::string arg = pArg;

		//if (_stricmp(pArg, "-Inheritance") == 0)
		//{
		//	foundInheritance = true;
		//}
	}

	//if (foundInheritance)
	//{
	//	Patch::Apply_RAW(0x528A10, // Apply INIClass_GetString_DisableAres
	//		{ 0x83, 0xEC, 0x0C, 0x33, 0xC0 }
	//	);

	//	Patch::Apply_RAW(0x526CC0, // Apply INIClass_GetKeyName_DisableAres
	//		{ 0x8B, 0x54, 0x24, 0x04, 0x83, 0xEC, 0x0C }
	//	);
	//}
	//else
	//{
	//	Patch::Apply_RAW(0x528BAC, // Revert INIClass_GetString_Inheritance_NoEntry
	//		{ 0x8B, 0x7C, 0x24, 0x2C, 0x33, 0xC0, 0x8B, 0x4C, 0x24, 0x28 }
	//	);
	//}

	Debug::Log("Initialized version: " PRODUCT_VERSION "\n");
}

void Phobos::ExeRun()
{
	Patch::ApplyStatic();
}

 // =============================
 // hooks

bool __stdcall DllMain(HANDLE hInstance, DWORD dwReason, LPVOID v)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		Phobos::hInstance = hInstance;
	}
	return true;
}

DEFINE_HOOK(0x7CD810, ExeRun, 0x9)
{
	Phobos::ExeRun();
	AresHelper::Init();

	return 0;
}

DEFINE_HOOK(0x52F639, _YR_CmdLineParse, 0x5)
{
	GET(char**, ppArgs, ESI);
	GET(int, nNumArgs, EDI);

	Phobos::CmdLineParse(ppArgs, nNumArgs);
	Debug::LogDeferredFinalize();
	return 0;
}

DEFINE_HOOK(0x67E44D, LoadGame_SetFlag, 0x5)
{
	Phobos::IsLoadingSaveGame = true;
	return 0;
}

DEFINE_HOOK(0x67E68A, LoadGame_UnsetFlag, 0x5)
{
	Phobos::IsLoadingSaveGame = false;
	Phobos::ApplyOptimizations();
	return 0;
}

DEFINE_HOOK(0x683E7F, ScenarioClass_Start_Optimizations, 0x7)
{
	Phobos::ApplyOptimizations();
	return 0;
}

// Mainly used to disable hooks for optimization.
// Called after loading saved game and at end of scenario start after all INI data etc has been initialized.
// Only executed once per game session.
void Phobos::ApplyOptimizations()
{
	if (Phobos::Optimizations::Applied)
		return;

	//// Disable BuildingClass_AI_Radiation
	//if (Phobos::Optimizations::DisableRadDamageOnBuildings)
	//	Patch::Apply_RAW(0x43FB23, { 0x53, 0x55, 0x56, 0x8B, 0xF1 });

	Phobos::Optimizations::Applied = true;
}
