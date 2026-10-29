#include <CCINIClass.h>
#include <Drawing.h>
#include <GameStrings.h>
#include <Helpers/Macro.h>
#include <HouseTypeClass.h>
#include <LoadProgressManager.h>
#include <SessionClass.h>
#include <Surface.h>

#include <Utilities/Constructs.h>
#include <Utilities/TemplateDef.h>

#include <cstdio>
#include <ctime>
#include <cwchar>
#include <memory>

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
	GET(CCINIClass*, pMapINI, EBP);

	// RulesExt has not been read yet, so the relevant INI data is read here.
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

	constexpr int assumedTextHeight = 15;
	constexpr int gapToBorder = 8;
	constexpr int gapToAnotherLine = 5;
	const int tipBarWidth = pRulesINI->ReadInteger(GameStrings::General, "TipBarWidth", 700);
	const bool tipBarTwoLines = pRulesINI->ReadBool(GameStrings::General, "TipBarTwoLines", false);
	const int width = tipBarWidth;
	const int height = tipBarTwoLines
		? (assumedTextHeight * 2 + gapToBorder * 2 + gapToAnotherLine)
		: (assumedTextHeight + gapToBorder * 2);
	const auto barRect = LoadProgressManager::Instance->LoadBarSHPRect;
	const int x = barRect.X + (barRect.Width - width) / 2;
	const int y = barRect.Y + barRect.Height - height - 5;
	auto rect = RectangleStruct { x, y, width, height };

	const auto pSurface = LoadProgressManager::Instance->ProgressSurface;
	pSurface->FillRect(&rect, COLOR_BLACK);
	pSurface->DrawRect(&rect, COLOR_WHITE);

	constexpr TextPrintType printType = TextPrintType::Center | TextPrintType::Point8;

	if (tipBarTwoLines)
	{
		const wchar_t* pFirstPart = pText;
		const wchar_t* pSecondPart = nullptr;
		std::unique_ptr<wchar_t[]> pFirstBuffer;

		if (const wchar_t* pNewline = wcschr(pText, L'\n'))
		{
			const size_t firstLen = pNewline - pText;
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
