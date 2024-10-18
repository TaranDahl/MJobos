#pragma once
#include <GameOptionsClass.h>
#include <EventClass.h>
#include <SuperClass.h>
#include <AircraftClass.h>
#include <MessageListClass.h>
#include <Ext/Side/Body.h>
#include <Ext/House/Body.h>
#include <Ext/SWType/Body.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/AresHelper.h>
#include <TacticalClass.h>
#include <WWMouseClass.h>
#include <CCToolTip.h>
#include <InputManagerClass.h>
#include <sstream>
#include <iomanip>

#include "PhobosToolTip.h"
#include "TacticalButtons.h"

TacticalButtonsClass TacticalButtonsClass::Instance;

// Keyboard code dictionary
static PhobosMap<int , const wchar_t*> CreateKeyboardCodeTextMap()
{
	PhobosMap<int , const wchar_t*> Code2Text;

	Code2Text[0x00] = L"  ";
	Code2Text[0x01] = L"MouseLeft";
	Code2Text[0x02] = L"MouseRight";
	Code2Text[0x03] = L"Cancel";
	Code2Text[0x04] = L"MouseCenter";

	Code2Text[0x08] = L"Back";
	Code2Text[0x09] = L"Tab";

	Code2Text[0x0C] = L"Clear";
	Code2Text[0x0D] = L"Enter";

	Code2Text[0x10] = L"Shift";
	Code2Text[0x11] = L"Ctrl";
	Code2Text[0x12] = L"Alt";
	Code2Text[0x13] = L"Pause";
	Code2Text[0x14] = L"CapsLock";

	Code2Text[0x1B] = L"Esc";

	Code2Text[0x20] = L"Space";
	Code2Text[0x21] = L"PageUp";
	Code2Text[0x22] = L"PageDown";
	Code2Text[0x23] = L"End";
	Code2Text[0x24] = L"Home";
	Code2Text[0x25] = L"Left";
	Code2Text[0x26] = L"Up";
	Code2Text[0x27] = L"Right";
	Code2Text[0x28] = L"Down";
	Code2Text[0x29] = L"Select";
	Code2Text[0x2A] = L"Print";
	Code2Text[0x2B] = L"Execute";
	Code2Text[0x2C] = L"PrintScreen";
	Code2Text[0x2D] = L"Insert";
	Code2Text[0x2E] = L"Delete";
	Code2Text[0x2F] = L"Help";
	Code2Text[0x30] = L"0";
	Code2Text[0x31] = L"1";
	Code2Text[0x32] = L"2";
	Code2Text[0x33] = L"3";
	Code2Text[0x34] = L"4";
	Code2Text[0x35] = L"5";
	Code2Text[0x36] = L"6";
	Code2Text[0x37] = L"7";
	Code2Text[0x38] = L"8";
	Code2Text[0x39] = L"9";

	Code2Text[0x41] = L"A";
	Code2Text[0x42] = L"B";
	Code2Text[0x43] = L"C";
	Code2Text[0x44] = L"D";
	Code2Text[0x45] = L"E";
	Code2Text[0x46] = L"F";
	Code2Text[0x47] = L"G";
	Code2Text[0x48] = L"H";
	Code2Text[0x49] = L"I";
	Code2Text[0x4A] = L"J";
	Code2Text[0x4B] = L"K";
	Code2Text[0x4C] = L"L";
	Code2Text[0x4D] = L"M";
	Code2Text[0x4E] = L"N";
	Code2Text[0x4F] = L"O";
	Code2Text[0x50] = L"P";
	Code2Text[0x51] = L"Q";
	Code2Text[0x52] = L"R";
	Code2Text[0x53] = L"S";
	Code2Text[0x54] = L"T";
	Code2Text[0x55] = L"U";
	Code2Text[0x56] = L"V";
	Code2Text[0x57] = L"W";
	Code2Text[0x58] = L"X";
	Code2Text[0x59] = L"Y";
	Code2Text[0x5A] = L"Z";
	Code2Text[0x5B] = L"LWin";
	Code2Text[0x5C] = L"RWin";
	Code2Text[0x5D] = L"Menu";

	Code2Text[0x60] = L"Num0";
	Code2Text[0x61] = L"Num1";
	Code2Text[0x62] = L"Num2";
	Code2Text[0x63] = L"Num3";
	Code2Text[0x64] = L"Num4";
	Code2Text[0x65] = L"Num5";
	Code2Text[0x66] = L"Num6";
	Code2Text[0x67] = L"Num7";
	Code2Text[0x68] = L"Num8";
	Code2Text[0x69] = L"Num9";
	Code2Text[0x6A] = L"Num*";
	Code2Text[0x6B] = L"Num+";
	Code2Text[0x6C] = L"Separator";
	Code2Text[0x6D] = L"Num-";
	Code2Text[0x6E] = L"Num.";
	Code2Text[0x6F] = L"Num/";
	Code2Text[0x70] = L"F1";
	Code2Text[0x71] = L"F2";
	Code2Text[0x72] = L"F3";
	Code2Text[0x73] = L"F4";
	Code2Text[0x74] = L"F5";
	Code2Text[0x75] = L"F6";
	Code2Text[0x76] = L"F7";
	Code2Text[0x77] = L"F8";
	Code2Text[0x78] = L"F9";
	Code2Text[0x79] = L"F10";
	Code2Text[0x7A] = L"F11";
	Code2Text[0x7B] = L"F12";

	Code2Text[0x90] = L"NumLock";
	Code2Text[0x91] = L"ScrollLock";

	Code2Text[0xBA] = L";";
	Code2Text[0xBB] = L"=";
	Code2Text[0xBC] = L",";
	Code2Text[0xBD] = L"-";
	Code2Text[0xBE] = L".";
	Code2Text[0xBF] = L"/";
	Code2Text[0xC0] = L"`";

	Code2Text[0xDB] = L"[";
	Code2Text[0xDC] = L"\\";
	Code2Text[0xDD] = L"]";
	Code2Text[0xDE] = L"'";

	Code2Text[static_cast<int>(WWKey::Shift)] = L"Shift";
	Code2Text[static_cast<int>(WWKey::Ctrl)] = L"Ctrl";
	Code2Text[static_cast<int>(WWKey::Alt)] = L"Alt";

	return Code2Text;
}

PhobosMap<int , const wchar_t*> TacticalButtonsClass::KeyboardCodeTextMap = CreateKeyboardCodeTextMap();

// Functions

// Private functions
int TacticalButtonsClass::CheckMouseOverButtons(const Point2D* pMousePosition)
{
	this->OnMessages = false;
	{
		const MessageListClass* pMessages = &MessageListClass::Instance;

		if (TextLabelClass* pText = pMessages->MessageList)
		{
			if (pMousePosition->Y >= pMessages->MessagePos.Y && pMousePosition->X >= pMessages->MessagePos.X && pMousePosition->X <= pMessages->MessagePos.X + pMessages->Width)
			{
				const int textHeight = pMessages->Height;
				int height = pMessages->MessagePos.Y;

				for (; pText; pText = static_cast<TextLabelClass*>(pText->GetNext()))
					height += textHeight;

				if (pMousePosition->Y < height)
					this->OnMessages = true;
			}
		}
	}

	if (const int currentCounts = HouseExt::ExtMap.Find(HouseClass::CurrentPlayer)->SWButtonData.size()) // Button index 1-11
	{
		const int height = DSurface::Composite->GetHeight() - 32;

		if (this->SuperVisible && pMousePosition->X < 65 && pMousePosition->X >= 5) // Button index 1-10 : Super weapons buttons
		{
			int checkHight = (height - 48 * currentCounts - 2 * (currentCounts - 1)) / 2;

			for (int i = 0; i < currentCounts; ++i)
			{
				if (pMousePosition->Y < checkHight)
					break;

				checkHight += 48;

				if (pMousePosition->Y < checkHight)
					return i + 1;

				checkHight += 2;
			}
		}

		if (RulesExt::Global()->SWSidebarBackground) // Button index 11 : SW sidebar switch
		{
			if (this->SuperVisible ? (pMousePosition->X < 90 && pMousePosition->X >= 80) : (pMousePosition->X < 10 && pMousePosition->X >= 0))
			{
				const int checkHight = height / 2;

				if (pMousePosition->Y < checkHight + 25 && pMousePosition->Y >= checkHight - 25)
					return 11;
			}
		}
	}

	// TODO New buttons

	if (Phobos::Config::SelectedDisplay_Enable) // Button index 71-100 : Select buttons
	{
		if (const int currentCounts = Math::min(30, static_cast<int>(this->CurrentSelectCameo.size())))
		{
			const int height = DSurface::Composite->GetHeight() - 80;

			if (pMousePosition->Y >= (this->IndexInSelectButtons() ? (height - 10) : height) && pMousePosition->Y < (height + 48))
			{
				const int gap = Math::min(60, ((DSurface::Composite->GetWidth() << 1) / (5 * currentCounts)));

				if (pMousePosition->X < (gap * (currentCounts - 1) + 60))
				{
					const int index = currentCounts + 71;
					int checkWidth = 0;

					for (int i = 71; i < index; ++i)
					{
						if (this->RecordIndex == i)
						{
							checkWidth += 60;

							if (this->ButtonIndex == this->RecordIndex)
							{
								if (pMousePosition->X < checkWidth)
									return i;

								break;
							}
							else if (pMousePosition->X < checkWidth && pMousePosition->Y >= height)
							{
								return i;
							}
						}
						else
						{
							checkWidth += gap;

							if (pMousePosition->X < checkWidth)
							{
								if (pMousePosition->Y >= height)
									return i;
								else
									break;
							}
						}
					}
				}
			}
		}
	}

	if (this->CheckMouseOverBackground(pMousePosition))
		return 0; // Button index 0 : Background

	return -1;
}

bool TacticalButtonsClass::CheckMouseOverBackground(const Point2D* pMousePosition)
{
	if (RulesExt::Global()->SWSidebarBackground && this->SuperVisible)
	{
		if (const int currentCounts = HouseExt::ExtMap.Find(HouseClass::CurrentPlayer)->SWButtonData.size())
		{
			if (pMousePosition->X < 80 && pMousePosition->X >= 0)
			{
				const int height = DSurface::Composite->GetHeight() - 32;
				const int checkHight = (height - 48 * currentCounts - 2 * (currentCounts - 1)) / 2 - 21;

				if (pMousePosition->Y >= checkHight && pMousePosition->Y < (checkHight + currentCounts * 50 + 40))
					return true;
			}
		}
	}

	// TODO New button backgrounds

	if (Phobos::Config::SelectedDisplay_Enable)
	{
		const int currentCounts = this->CurrentSelectCameo.size();

		if (currentCounts == 1 && this->CurrentSelectCameo[0].Count == 1)
		{
			if (pMousePosition->X < 180 && pMousePosition->X >= 0)
			{
				const int height = DSurface::Composite->GetHeight() - 80;

				if (pMousePosition->Y >= height && pMousePosition->Y < (height + 48))
					return true;
			}
		}
	}

	return false;
}

// Inline functions
inline bool TacticalButtonsClass::MouseIsOverButtons()
{
	return this->ButtonIndex > 0;
}

inline bool TacticalButtonsClass::MouseIsOverTactical()
{
	return this->ButtonIndex < 0;
}

// Cite functions
int TacticalButtonsClass::GetButtonIndex()
{
	return this->ButtonIndex;
}

// General functions
void TacticalButtonsClass::SetMouseButtonIndex(const Point2D* pMousePosition)
{
	this->ButtonIndex = this->CheckMouseOverButtons(pMousePosition);

	CCToolTip* const toolTips = CCToolTip::Instance;

	// SW ToolTip
	if (this->IndexInSWButtons()) // Button index 1-10 : Super weapons buttons
	{
		HouseClass* const pHouse = HouseClass::CurrentPlayer;
		SuperClass* const pSuper = pHouse->Supers.Items[HouseExt::ExtMap.Find(pHouse)->SWButtonData[this->ButtonIndex - 1]];

		if (pSuper && pSuper != this->RecordSuper)
		{
			PhobosToolTip::Instance.HelpText(pSuper);
			this->RecordSuper = pSuper;

			if (toolTips->ToolTipDelay)
				toolTips->LastToolTipDelay = toolTips->ToolTipDelay;

			toolTips->ToolTipDelay = 0;
		}
	}
	else if (this->RecordSuper)
	{
		this->RecordSuper = nullptr;
		toolTips->ToolTipDelay = toolTips->LastToolTipDelay;
	}

	// TODO New buttons

	// Select ToolTip And Button Record
	if (this->IndexInSelectButtons()) // Button index 71-100 : Select buttons
	{
		this->RecordIndex = this->ButtonIndex;
		const int currentCounts = this->CurrentSelectCameo.size();

		if (currentCounts > 1 || (currentCounts && this->CurrentSelectCameo[0].Count > 1))
		{
			if (TechnoTypeClass* const pObj = this->CurrentSelectCameo[this->ButtonIndex - 71].TypeExt->OwnerObject())
			{
				const wchar_t* name = pObj->UIName;

				if (name && name != this->HoveredSelected)
					this->HoveredSelected = name;
			}
		}
		else if (this->HoveredSelected)
		{
			this->HoveredSelected = nullptr;
		}
	}
	else if (this->HoveredSelected)
	{
		this->HoveredSelected = nullptr;
	}
}

void TacticalButtonsClass::PressDesignatedButton(int triggerIndex)
{
	if (!this->MouseIsOverButtons()) // In buttons background
		return;

	if (this->IndexInSWButtons()) // Button index 1-10 : Super weapons buttons
	{
		if (!triggerIndex)
			this->SWSidebarTrigger(this->ButtonIndex);
		else if (triggerIndex == 2)
			DisplayClass::Instance->CurrentSWTypeIndex = -1;

		CCToolTip* const toolTips = CCToolTip::Instance;
		this->RecordSuper = nullptr;
		toolTips->ToolTipDelay = toolTips->LastToolTipDelay;
	}
	else if (this->IndexIsSWSwitch())
	{
		if (!triggerIndex)
			this->SWSidebarSwitch();
	}
/*	else if (?) // TODO New buttons
	{
		;
	}*/
	else if (this->IndexInSelectButtons())
	{
		if (!triggerIndex)
			this->SelectedTrigger(this->ButtonIndex, true);
		else if (triggerIndex == 2)
			this->SelectedTrigger(this->ButtonIndex, false);
	}
}

// Button index 1-10 : Super weapons buttons
inline bool TacticalButtonsClass::IndexInSWButtons()
{
	return this->ButtonIndex > 0 && this->ButtonIndex <= 10;
}

void TacticalButtonsClass::SWSidebarDraw()
{
	HouseClass* const pHouse = HouseClass::CurrentPlayer;
	HouseExt::ExtData* const pHouseExt = HouseExt::ExtMap.Find(pHouse);
	const int currentCounts = pHouseExt->SWButtonData.size();

	if (!currentCounts)
		return;

	SideExt::ExtData* const pSideExt = SideExt::ExtMap.Find(SideClass::Array->GetItem(pHouse->SideIndex));
	const bool drawSWSidebarBackground = RulesExt::Global()->SWSidebarBackground && pSideExt;
	const int height = DSurface::Composite->GetHeight() - 32;
	const int color = Drawing::RGB_To_Int(Drawing::TooltipColor);

	// Draw switch
	if (this->SuperVisible)
	{
		if (drawSWSidebarBackground)
		{
			RectangleStruct drawRect { 80, (height / 2 - 25), 10, 50 };

			if (BSurface* const CameoPCX = pSideExt->SWSidebarBackground_OnPCX.GetSurface())
				PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
			else
				DSurface::Composite->FillRect(&drawRect, COLOR_BLUE);

			if (this->IndexIsSWSwitch())
			{
				RectangleStruct rect { 0, 0, 90, drawRect.Y + 50 };
				DSurface::Composite->DrawRectEx(&rect, &drawRect, color);
			}
		}
	}
	else
	{
		if (drawSWSidebarBackground)
		{
			RectangleStruct drawRect { 0, (height / 2 - 25), 10, 50 };

			if (BSurface* const CameoPCX = pSideExt->SWSidebarBackground_OffPCX.GetSurface())
				PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
			else
				DSurface::Composite->FillRect(&drawRect, COLOR_BLUE);

			if (this->IndexIsSWSwitch())
			{
				RectangleStruct rect { 0, 0, 10, drawRect.Y + 50 };
				DSurface::Composite->DrawRectEx(&rect, &drawRect, color);
			}
		}

		return;
	}

	auto& data = pHouseExt->SWButtonData;
	Point2D position { 5, (height - 48 * currentCounts - 2 * (currentCounts - 1)) / 2 };
	RectangleStruct rect { 0, 0, 65, position.Y + 48 };
	int recordHeight = -1;

	// Draw top background (80 * 20)
	Point2D backPosition { 0, position.Y - 21 };

	if (drawSWSidebarBackground)
	{
		RectangleStruct drawRect { 0, backPosition.Y, 80, 20 };

		if (BSurface* const CameoPCX = pSideExt->SWSidebarBackground_TopPCX.GetSurface())
			PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
		else
			DSurface::Composite->FillRect(&drawRect, COLOR_BLACK);
	}

	// Draw each buttons
	for (int i = 0; i < currentCounts; position.Y += 50, rect.Height += 50) // Button index 1-10
	{
		// Draw center background (80 * 50)
		if (drawSWSidebarBackground)
		{
			backPosition.Y = position.Y - 1;
			RectangleStruct drawRect { 0, backPosition.Y, 80, 50 };

			if (BSurface* const CameoPCX = pSideExt->SWSidebarBackground_CenterPCX.GetSurface())
				PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
			else
				DSurface::Composite->FillRect(&drawRect, COLOR_BLACK);
		}

		// Get SW data
		SuperClass* const pSuper = pHouse->Supers.Items[data[i]];
		SuperWeaponTypeClass* const pSWType = pSuper->Type;

		// Draw cameo
		if (SWTypeExt::ExtData* const pTypeExt = SWTypeExt::ExtMap.Find(pSWType))
		{
			BSurface* const CameoPCX = pTypeExt->SidebarPCX.GetSurface();

			if (CAN_USE_ARES && AresHelper::CanUseAres && CameoPCX)
			{
				RectangleStruct drawRect { position.X, position.Y, 60, 48 };
				PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
			}
			else if (SHPStruct* const pSHP = pSWType->SidebarImage)
			{
				SHPReference* const pCameoRef = pSHP->AsReference();

				char pFilename[0x20];
				strcpy_s(pFilename, RulesExt::Global()->MissingCameo.data());
				_strlwr_s(pFilename);

				if (!_stricmp(pCameoRef->Filename, GameStrings::XXICON_SHP) && strstr(pFilename, ".pcx"))
				{
					PCX::Instance->LoadFile(pFilename);
					RectangleStruct drawRect { position.X, position.Y, 60, 48 };

					if (BSurface* const MissingCameoPCX = PCX::Instance->GetSurface(pFilename))
						PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
				}
				else
				{
					DSurface::Composite->DrawSHP(pTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
						BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
				}
			}
		}

		const bool ready = !pSuper->IsSuspended && (pSWType->UseChargeDrain ? pSuper->ChargeDrainState == ChargeDrainState::Ready : pSuper->IsReady);

		// Flash cameo
		if (ready)
		{
			const int delay = pSWType->FlashSidebarTabFrames;

			if (delay > 0 && ((Unsorted::CurrentFrame - pSuper->ReadyFrame) % (delay << 1)) > delay)
			{
				DSurface::Composite->DrawSHP(FileSystem::SIDEBAR_PAL, Make_Global<SHPStruct*>(0xB07BC0), 0, &position, &rect,
					BlitterFlags(0x406), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
			}
		}

		// SW charge progress
		if (pSuper->ShouldDrawProgress())
		{
			const int frame = pSuper->AnimStage() + 1;

			DSurface::Composite->DrawSHP(FileSystem::SIDEBAR_PAL, Make_Global<SHPStruct*>(0xB0B484), frame, &position, &rect,
				BlitterFlags(0x404), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}

		// SW status
		if (ready && !this->KeyCodeText[i].empty())
		{
			const wchar_t* pKey = this->KeyCodeText[i].c_str();
			Point2D textLocation { 35, position.Y + 1 };
			const TextPrintType printType = TextPrintType::Center | TextPrintType::FullShadow | TextPrintType::Point8;
			RectangleStruct textRect = Drawing::GetTextDimensions(pKey, textLocation, static_cast<WORD>(printType), 2, 1);

			// Text black background
			reinterpret_cast<void(__fastcall*)(RectangleStruct*, DSurface*, unsigned short, unsigned char)>(0x621B80)(&textRect, DSurface::Composite, 0, 0xAFu);
			DSurface::Composite->DrawTextA(pKey, &rect, &textLocation, static_cast<COLORREF>(color), COLOR_BLACK, printType);
		}
		else if (const wchar_t* pName = pSuper->NameReadiness())
		{
			Point2D textLocation { 35, position.Y + 1 };
			const TextPrintType printType = TextPrintType::Center | TextPrintType::FullShadow | TextPrintType::Point8;
			RectangleStruct textRect = Drawing::GetTextDimensions(pName, textLocation, static_cast<WORD>(printType), 2, 1);

			// Text black background
			reinterpret_cast<void(__fastcall*)(RectangleStruct*, DSurface*, unsigned short, unsigned char)>(0x621B80)(&textRect, DSurface::Composite, 0, 0xAFu);
			DSurface::Composite->DrawTextA(pName, &rect, &textLocation, static_cast<COLORREF>(color), COLOR_BLACK, printType);
		}

		if (++i == this->ButtonIndex)
			recordHeight = position.Y;
	}

	// Draw bottom background (80 * 20)
	if (drawSWSidebarBackground)
	{
		backPosition.Y = position.Y - 1;
		RectangleStruct drawRect { 0, backPosition.Y, 80, 20};

		if (BSurface* const CameoPCX = pSideExt->SWSidebarBackground_BottomPCX.GetSurface())
			PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
		else
			DSurface::Composite->FillRect(&drawRect, COLOR_BLACK);
	}

	// Draw mouse hover rectangle
	if (!ScenarioClass::Instance->UserInputLocked && recordHeight >= 0)
	{
		position.Y = recordHeight;
		rect.Height = recordHeight + 48;

		RectangleStruct drawRect { 5, position.Y, 60, 48 };
		DSurface::Composite->DrawRectEx(&rect, &drawRect, color);
	}
}

void TacticalButtonsClass::SWSidebarRecheck()
{
	HouseClass* const pHouse = HouseClass::CurrentPlayer;
	auto& data = HouseExt::ExtMap.Find(pHouse)->SWButtonData;

	for (auto it = data.begin(); it != data.end();)
	{
		const int superIndex = *it;

		if (superIndex >= pHouse->Supers.Count || !pHouse->Supers.Items[superIndex]->IsPresent)
			it = data.erase(it);
		else
			++it;
	}
}

bool TacticalButtonsClass::SWSidebarAdd(int& superIndex)
{
	HouseClass* const pHouse = HouseClass::CurrentPlayer;
	SuperWeaponTypeClass* const pType = SuperWeaponTypeClass::Array->Items[superIndex];
	SWTypeExt::ExtData* const pTypeExt = SWTypeExt::ExtMap.Find(pType);
	bool overflow = true;

	if (pTypeExt && pTypeExt->SW_InScreen_Show)
	{
		const unsigned int ownerBits = 1u << pHouse->Type->ArrayIndex;

		if (pTypeExt->SW_InScreen_RequiredHouses & ownerBits)
		{
			HouseExt::ExtData* const pHouseExt = HouseExt::ExtMap.Find(pHouse);
			const int currentCounts = pHouseExt->SWButtonData.size();
			auto& data = pHouseExt->SWButtonData;
			bool move = false;

			for (int i = 0; i < 10; ++i) // 10 buttons at max
			{
				if (move)
				{
					if (i < currentCounts)
					{
						int buffer = data[i];
						data[i] = superIndex;
						superIndex = buffer;
					}
					else
					{
						data.push_back(superIndex);
						overflow = false;
						break;
					}
				}
				else if (i < currentCounts)
				{
					if (this->SWSidebarSort(SuperWeaponTypeClass::Array->Items[data[i]], pType, pTypeExt, ownerBits))
					{
						move = true;
						int buffer = data[i];
						data[i] = superIndex;
						superIndex = buffer;
					}
				}
				else
				{
					data.push_back(superIndex);
					overflow = false;
					break;
				}
			}
		}
	}

	return overflow;
}

bool TacticalButtonsClass::SWSidebarSort(SuperWeaponTypeClass* pDataType, SuperWeaponTypeClass* pAddType, SWTypeExt::ExtData* pAddTypeExt, unsigned int ownerBits)
{
	SWTypeExt::ExtData* const pDataTypeExt = SWTypeExt::ExtMap.Find(pDataType);

	if (pDataTypeExt && pAddTypeExt)
	{
		if ((pDataTypeExt->SW_InScreen_PriorityHouses & ownerBits) && !(pAddTypeExt->SW_InScreen_PriorityHouses & ownerBits))
			return false;

		if (!(pDataTypeExt->SW_InScreen_PriorityHouses & ownerBits) && (pAddTypeExt->SW_InScreen_PriorityHouses & ownerBits))
			return true;

		if (pDataTypeExt->CameoPriority > pAddTypeExt->CameoPriority)
			return false;

		if (pDataTypeExt->CameoPriority < pAddTypeExt->CameoPriority)
			return true;
	}

	if (pDataType->RechargeTime < pAddType->RechargeTime)
		return false;

	if (pDataType->RechargeTime > pAddType->RechargeTime)
		return true;

	return wcscmp(pDataType->UIName, pAddType->UIName) > 0;
}

void TacticalButtonsClass::SWSidebarTrigger(int buttonIndex)
{
	if (ScenarioClass::Instance->UserInputLocked || !this->SuperVisible)
		return;

	HouseClass* const pHouse = HouseClass::CurrentPlayer;

	if (pHouse->IsObserver())
		return;

	HouseExt::ExtData* const pHouseExt = HouseExt::ExtMap.Find(pHouse);

	if (static_cast<size_t>(buttonIndex) > pHouseExt->SWButtonData.size())
		return;

	SidebarClass* const pSidebar = SidebarClass::Instance;
	this->DummyAction = true;

	DummySelectClass pButton;
	pButton.LinkTo = &pSidebar->Tabs[pSidebar->ActiveTabIndex];
	pButton.unknown_int_30 = 0x7FFFFFFF - (2 * pButton.LinkTo->TopRowIndex);
	pButton.SWIndex = pHouseExt->SWButtonData[buttonIndex - 1];

	DWORD KeyNum = 0;
	reinterpret_cast<bool(__thiscall*)(DummySelectClass*, GadgetFlag, DWORD*, KeyModifier)>(0x6AAD00)(&pButton, GadgetFlag::LeftPress, &KeyNum, KeyModifier::None); // SelectClass_Action
}

void TacticalButtonsClass::SWSidebarRecord(int buttonIndex, int key)
{
	const int index = buttonIndex - 1;

	if (this->KeyCodeData[index] == key)
		return;

	this->KeyCodeData[index] = key;
	std::wostringstream oss;

	if (key & static_cast<int>(WWKey::Shift))
		oss << KeyboardCodeTextMap[static_cast<int>(WWKey::Shift)] << L"+";

	if (key & static_cast<int>(WWKey::Ctrl))
		oss << KeyboardCodeTextMap[static_cast<int>(WWKey::Ctrl)] << L"+";

	if (key & static_cast<int>(WWKey::Alt))
		oss << KeyboardCodeTextMap[static_cast<int>(WWKey::Alt)] << L"+";

	const int pureKey = key & 0xFF;

	if (KeyboardCodeTextMap.contains(pureKey))
		oss << KeyboardCodeTextMap[pureKey];
	else
		oss << L"Unknown";

	this->KeyCodeText[index] = oss.str();
}

// Button index 11 : SW sidebar switch
inline bool TacticalButtonsClass::IndexIsSWSwitch()
{
	return this->ButtonIndex == 11;
}

void TacticalButtonsClass::SWSidebarSwitch()
{
	if (ScenarioClass::Instance->UserInputLocked || HouseClass::CurrentPlayer->IsObserver())
		return;

	this->SuperVisible = !this->SuperVisible;

	MessageListClass::Instance->PrintMessage
	(
		(this->SuperVisible ?
			GeneralUtils::LoadStringUnlessMissing("TXT_EX_SW_BAR_VISIBLE", L"Set exclusive SW sidebar visible.") :
			GeneralUtils::LoadStringUnlessMissing("TXT_EX_SW_BAR_INVISIBLE", L"Set exclusive SW sidebar invisible.")),
		RulesClass::Instance->MessageDelay,
		HouseClass::CurrentPlayer->ColorSchemeIndex,
		true
	);
}

// Extra functions for SW
bool TacticalButtonsClass::SWQuickLaunch(int superIndex)
{
	bool keyboardCall = false;

	if (this->KeyboardCall)
	{
		this->KeyboardCall = false;
		keyboardCall = true;
	}

	SuperWeaponTypeClass* const pType = SuperWeaponTypeClass::Array->Items[superIndex];

	if (SWTypeExt::ExtData* const pTypeExt = SWTypeExt::ExtMap.Find(pType))
	{
		if (pTypeExt->SW_QuickFireAtMouse && keyboardCall)
		{
			const CoordStruct mouseCoords = TacticalClass::Instance->ClientToCoords(WWMouseClass::Instance->XY1);

			if (mouseCoords != CoordStruct::Empty)
			{
				EventClass event
				(
					HouseClass::CurrentPlayer->ArrayIndex,
					EventType::SpecialPlace,
					pType->ArrayIndex,
					CellClass::Coord2Cell(mouseCoords)
				);
				EventClass::AddEvent(event);

				return true;
			}
		}
		else if (!pTypeExt->SW_QuickFireInScreen)
		{
			return false;
		}

		EventClass event
		(
			HouseClass::CurrentPlayer->ArrayIndex,
			EventType::SpecialPlace,
			pType->ArrayIndex,
			CellClass::Coord2Cell(TacticalClass::Instance->ClientToCoords(Point2D{ (DSurface::Composite->Width >> 1), (DSurface::Composite->Height >> 1) }))
		);
		EventClass::AddEvent(event);

		return true;
	}

	return false;
}

// Button index 71-100 : Select buttons
inline bool TacticalButtonsClass::IndexInSelectButtons()
{
	return this->ButtonIndex > 70 && this->ButtonIndex <= 100;
}

inline void TacticalButtonsClass::AddToCurrentSelect(TechnoTypeExt::ExtData* pTypeExt, int count, int checkIndex)
{
	const char* groupID = pTypeExt->GetSelectionGroupID();
	const int currentCounts = this->CurrentSelectCameo.size();

	for (int i = checkIndex; i < currentCounts; ++i)
	{
		if (this->CurrentSelectCameo[i].TypeExt->GetSelectionGroupID() == groupID)
		{
			this->CurrentSelectCameo[i].Count += count;
			return;
		}
	}

	SelectRecordStruct select { pTypeExt, count };
	this->CurrentSelectCameo.push_back(select);
}

void TacticalButtonsClass::SelectedTrigger(int buttonIndex, bool select)
{
	if (ScenarioClass::Instance->UserInputLocked || !Phobos::Config::SelectedDisplay_Enable)
		return;

	const int currentCounts = this->CurrentSelectCameo.size();

	if (buttonIndex > (currentCounts + 70))
		return;

	TechnoTypeExt::ExtData* const pTypeExt = this->CurrentSelectCameo[buttonIndex - 71].TypeExt;
	const char* groupID = pTypeExt->GetSelectionGroupID();
	const bool shiftPressed = InputManagerClass::Instance->IsForceSelectKeyPressed();

	if (select)
	{
		if (shiftPressed)
		{
			std::vector<ObjectClass*> deselects;
			deselects.reserve(ObjectClass::CurrentObjects->Count);

			for (auto const& pCurrent : ObjectClass::CurrentObjects())
			{
				if (TechnoTypeExt::ExtData* const pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() == groupID)
						continue;
				}

				deselects.push_back(pCurrent);
			}

			for (auto const& pDeselect : deselects)
			{
				pDeselect->Deselect();
			}
		}
		else
		{
			const int counts = ObjectClass::CurrentObjects->Count;
			std::vector<ObjectClass*> selects;
			selects.reserve(counts);
			std::vector<ObjectClass*> deselects;
			deselects.reserve(counts);

			for (auto const& pCurrent : ObjectClass::CurrentObjects())
			{
				if (TechnoTypeExt::ExtData* const pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() == groupID)
					{
						selects.push_back(pCurrent);
						continue;
					}
				}

				deselects.push_back(pCurrent);
			}

			for (auto const& pDeselect : deselects)
			{
				pDeselect->Deselect();
			}

			const int size = selects.size();
			const int random = Unsorted::CurrentFrame % size;

			for (int i = 0; i < size; ++i)
			{
				if (i != random)
					selects[i]->Deselect();
			}
		}
	}
	else
	{
		if (shiftPressed)
		{
			std::vector<ObjectClass*> deselects;
			deselects.reserve(ObjectClass::CurrentObjects->Count);

			for (auto const& pCurrent : ObjectClass::CurrentObjects())
			{
				if (TechnoTypeExt::ExtData* const pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() != groupID)
						continue;
				}

				deselects.push_back(pCurrent);
			}

			for (auto const& pDeselect : deselects)
			{
				pDeselect->Deselect();
			}
		}
		else
		{
			std::vector<ObjectClass*> selects;
			selects.reserve(ObjectClass::CurrentObjects->Count);

			for (auto const& pCurrent : ObjectClass::CurrentObjects())
			{
				if (TechnoTypeExt::ExtData* const pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() == groupID)
						selects.push_back(pCurrent);
				}
			}

			selects[Unsorted::CurrentFrame % selects.size()]->Deselect();
		}
	}
}

void TacticalButtonsClass::SelectedUpdate()
{
	if (this->CurrentSelectCameo.size())
		this->CurrentSelectCameo.clear();

	if (!Phobos::Config::SelectedDisplay_Enable || ObjectClass::CurrentObjects->Count <= 0) // TODO Optimize
	{
		this->RecordIndex = 71;
		return;
	}

	this->CurrentSelectCameo.reserve(10);

	std::map<int, int> CurrentSelectInfantry;
	std::map<int, int> CurrentSelectUnit;
	std::map<int, int> CurrentSelectAircraft;
	std::map<int, int> CurrentSelectBuilding;

	for (auto const& pCurrent : ObjectClass::CurrentObjects())
	{
		const AbstractType absType = pCurrent->WhatAmI();

		if (absType == AbstractType::Infantry)
			++CurrentSelectInfantry[static_cast<InfantryClass*>(pCurrent)->Type->ArrayIndex];
		if (absType == AbstractType::Unit)
			++CurrentSelectUnit[static_cast<UnitClass*>(pCurrent)->Type->ArrayIndex];
		if (absType == AbstractType::Aircraft)
			++CurrentSelectAircraft[static_cast<AircraftClass*>(pCurrent)->Type->ArrayIndex];
		if (absType == AbstractType::Building)
			++CurrentSelectBuilding[static_cast<BuildingClass*>(pCurrent)->Type->ArrayIndex];
	}

	if (CurrentSelectInfantry.size())
	{
		for (auto const& [index, count] : CurrentSelectInfantry)
		{
			if (TechnoTypeExt::ExtData* const pTypeExt = TechnoTypeExt::ExtMap.Find(InfantryTypeClass::Array->Items[index]))
				this->AddToCurrentSelect(pTypeExt, count, 0);
		}
	}

	if (CurrentSelectUnit.size())
	{
		const int checkStart = this->CurrentSelectCameo.size();

		for (auto const& [index, count] : CurrentSelectUnit)
		{
			if (TechnoTypeExt::ExtData* const pTypeExt = TechnoTypeExt::ExtMap.Find(UnitTypeClass::Array->Items[index]))
				this->AddToCurrentSelect(pTypeExt, count, checkStart);
		}
	}

	if (CurrentSelectAircraft.size())
	{
		const int checkStart = this->CurrentSelectCameo.size();

		for (auto const& [index, count] : CurrentSelectAircraft)
		{
			if (TechnoTypeExt::ExtData* const pTypeExt = TechnoTypeExt::ExtMap.Find(AircraftTypeClass::Array->Items[index]))
				this->AddToCurrentSelect(pTypeExt, count, checkStart);
		}
	}

	if (CurrentSelectBuilding.size())
	{
		const int checkStart = this->CurrentSelectCameo.size();

		for (auto const& [index, count] : CurrentSelectBuilding)
		{
			if (TechnoTypeExt::ExtData* const pTypeExt = TechnoTypeExt::ExtMap.Find(BuildingTypeClass::Array->Items[index]))
				this->AddToCurrentSelect(pTypeExt, count, checkStart);
		}
	}

	const int maxIndex = this->CurrentSelectCameo.size() + 70;

	if (this->RecordIndex > maxIndex)
		this->RecordIndex = maxIndex;
}

void TacticalButtonsClass::SelectedDraw()
{
	if (!Phobos::Config::SelectedDisplay_Enable)
		return;

	const int currentCounts = Math::min(30, static_cast<int>(this->CurrentSelectCameo.size()));

	if (currentCounts > 1 || (currentCounts && this->CurrentSelectCameo[0].Count > 1)) // Mass
	{
		Point2D position { 0, DSurface::Composite->GetHeight() - 80 };
		Point2D textPosition { 1, position.Y + 1 };
		RectangleStruct drawRect { 0, position.Y, 60, 48 };

		const int gap = Math::min(60, ((DSurface::Composite->GetWidth() << 1) / (5 * currentCounts)));
		RectangleStruct surfaceRect { 0, 0, (gap * (currentCounts - 1) + 60), (position.Y + 48) };

		const int maxIndex = currentCounts + 70;
		const COLORREF color = Drawing::RGB_To_Int(Drawing::TooltipColor);
		TextPrintType printType = TextPrintType::Background | TextPrintType::FullShadow | TextPrintType::Point8;

		for (int i = 71; i < this->RecordIndex && i <= maxIndex; ++i, position.X += gap, drawRect.X = position.X, textPosition.X += gap)
		{
			SelectRecordStruct pSelect = this->CurrentSelectCameo[i - 71];

			if (BSurface* const CameoPCX = pSelect.TypeExt->CameoPCX.GetSurface())
			{
				PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
			}
			else if (SHPStruct* const pSHP = pSelect.TypeExt->OwnerObject()->GetCameo())
			{
				SHPReference* const pCameoRef = pSHP->AsReference();
				char pFilename[0x20];
				strcpy_s(pFilename, RulesExt::Global()->MissingCameo.data());
				_strlwr_s(pFilename);

				if (!_stricmp(pCameoRef->Filename, GameStrings::XXICON_SHP) && strstr(pFilename, ".pcx"))
				{
					PCX::Instance->LoadFile(pFilename);

					if (BSurface* const MissingCameoPCX = PCX::Instance->GetSurface(pFilename))
						PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
				}
				else
				{
					RectangleStruct rect { 0, 0, (position.X + 60), surfaceRect.Height };
					DSurface::Composite->DrawSHP(pSelect.TypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
						BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
				}
			}

			const int count = pSelect.Count;

			if (count > 1)
			{
				wchar_t text[0x20];
				swprintf_s(text, L"%d", count);
				DSurface::Composite->DrawTextA(text, &surfaceRect, &textPosition, color, 0, printType);
			}
		}

		position.X = surfaceRect.Width - 60;
		drawRect.X = position.X;

		printType |= TextPrintType::Right;
		textPosition.X = surfaceRect.Width - 1;

		for (int i = maxIndex; i > this->RecordIndex; --i, position.X -= gap, drawRect.X = position.X, textPosition.X -= gap)
		{
			SelectRecordStruct pSelect = this->CurrentSelectCameo[i - 71];

			if (BSurface* const CameoPCX = pSelect.TypeExt->CameoPCX.GetSurface())
			{
				PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
			}
			else if (SHPStruct* const pSHP = pSelect.TypeExt->OwnerObject()->GetCameo())
			{
				SHPReference* const pCameoRef = pSHP->AsReference();
				char pFilename[0x20];
				strcpy_s(pFilename, RulesExt::Global()->MissingCameo.data());
				_strlwr_s(pFilename);

				if (!_stricmp(pCameoRef->Filename, GameStrings::XXICON_SHP) && strstr(pFilename, ".pcx"))
				{
					PCX::Instance->LoadFile(pFilename);

					if (BSurface* const MissingCameoPCX = PCX::Instance->GetSurface(pFilename))
						PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
				}
				else
				{
					RectangleStruct rect { 0, 0, (position.X + 60), surfaceRect.Height };
					DSurface::Composite->DrawSHP(pSelect.TypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
						BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
				}
			}

			const int count = pSelect.Count;

			if (count > 1)
			{
				wchar_t text[0x20];
				swprintf_s(text, L"%d", count);
				DSurface::Composite->DrawTextA(text, &surfaceRect, &textPosition, color, 0, printType);
			}
		}

		const bool hovered = this->ButtonIndex == this->RecordIndex;

		if (hovered)
		{
			position.Y -= 10;
			drawRect.Y -= 10;
			textPosition.Y -= 10;
		}

		if (this->RecordIndex <= maxIndex)
		{
			SelectRecordStruct pSelect = this->CurrentSelectCameo[this->RecordIndex - 71];

			if (BSurface* const CameoPCX = pSelect.TypeExt->CameoPCX.GetSurface())
			{
				PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
			}
			else if (SHPStruct* const pSHP = pSelect.TypeExt->OwnerObject()->GetCameo())
			{
				SHPReference* const pCameoRef = pSHP->AsReference();
				char pFilename[0x20];
				strcpy_s(pFilename, RulesExt::Global()->MissingCameo.data());
				_strlwr_s(pFilename);

				if (!_stricmp(pCameoRef->Filename, GameStrings::XXICON_SHP) && strstr(pFilename, ".pcx"))
				{
					PCX::Instance->LoadFile(pFilename);

					if (BSurface* const MissingCameoPCX = PCX::Instance->GetSurface(pFilename))
						PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
				}
				else
				{
					RectangleStruct rect { 0, 0, (position.X + 60), (hovered ? (surfaceRect.Height - 10) : surfaceRect.Height) };
					DSurface::Composite->DrawSHP(pSelect.TypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
						BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
				}
			}

			const int count = pSelect.Count;

			if (count > 1)
			{
				wchar_t text[0x20];
				swprintf_s(text, L"%d", count);
				DSurface::Composite->DrawTextA(text, &surfaceRect, &textPosition, color, 0, printType);
			}
		}
	}
	else if (currentCounts) // Only 1
	{
		for (auto const& pCurrent : ObjectClass::CurrentObjects())
		{
			if (TechnoClass* const pThis = abstract_cast<TechnoClass*>(pCurrent))
			{
				TechnoTypeClass* const pType = pThis->GetTechnoType();
				TechnoTypeExt::ExtData* const pTypeExt = this->CurrentSelectCameo[0].TypeExt;

				if (pTypeExt->OwnerObject() == pType)
				{
					TextPrintType printType = TextPrintType::Center | TextPrintType::Point8;
					COLORREF color = Drawing::RGB_To_Int(Drawing::TooltipColor);
					RectangleStruct drawRect { 0, DSurface::Composite->GetHeight() - 80, 180, 48};
					{
						DSurface::Composite->FillRect(&drawRect, COLOR_BLACK);
						drawRect.X += 60;
						drawRect.Width = 120;
						DSurface::Composite->DrawRect(&drawRect, color);
						drawRect.X -= 60;
					}
					drawRect.Width = 60;
					Point2D position { 60, drawRect.Y };
					RectangleStruct surfaceRect { 0, 0, 180, position.Y + 48 };

					BSurface* CameoPCX = pTypeExt->CameoPCX.GetSurface();
					SHPStruct* pSHP = pType->GetCameo();
					ConvertClass* pPal = pTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL);

					position += Point2D { 60, 3 };
					{
						HouseClass* const pHouse = HouseClass::CurrentPlayer;
						const wchar_t* name = pType->UIName;

						if (!pHouse->IsAlliedWith(pThis) && !pHouse->IsObserver())
						{
							if (TechnoTypeClass* const pFakeType = pTypeExt->FakeOf)
							{
								if (TechnoTypeExt::ExtData* const pFakeTypeExt = TechnoTypeExt::ExtMap.Find(pFakeType))
								{
									if (const wchar_t* fakeName = pFakeTypeExt->EnemyUIName.Get().Text)
										name = fakeName;
									else
										name = pFakeType->UIName;

									if (BSurface* const FakeCameoPCX = pFakeTypeExt->CameoPCX.GetSurface())
									{
										CameoPCX = FakeCameoPCX;
									}
									else if (SHPStruct* const pFakeSHP = pFakeType->GetCameo())
									{
										pSHP = pFakeSHP;
										pPal = pFakeTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL);
									}
								}
								else
								{
									name = pFakeType->UIName;
								}
							}
							else if (const wchar_t* enemyName = pTypeExt->EnemyUIName.Get().Text)
							{
								name = enemyName;
							}
						}

						if (name)
						{
							size_t length = Math::min(wcslen(name), static_cast<size_t>(31));

							for (const wchar_t* check = name; *check != L'\0'; ++check)
							{
								if (*check == L'\n')
								{
									length = static_cast<size_t>(check - name);
									break;
								}
							}

							wchar_t text1[0x20] = {0};
							wcsncpy_s(text1, name, length);
							text1[length] = L'\0';
							DSurface::Composite->DrawTextA(text1, &surfaceRect, &position, color, 0, printType);
						}
					}

					position.Y += 15;
					{
						int value = -1, maxValue = 0;
						TechnoExt::GetValuesForDisplay(pThis, pTypeExt->UpperSelectedInfoType, value, maxValue);

						if (value >= 0 && maxValue > 0)
						{
							if (pTypeExt->UpperSelectedInfoColor.Get() == ColorStruct{ 0, 0, 0 })
							{
								RulesClass* const pRules = RulesClass::Instance;
								const double ratio = static_cast<double>(value) / maxValue;
								color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
							}
							else
							{
								color = Drawing::RGB_To_Int(pTypeExt->UpperSelectedInfoColor);
							}

							wchar_t text2[0x20] = {0};

							position.X += 10;
							printType &= ~TextPrintType::Center;
							swprintf_s(text2, L"%d", maxValue);
							DSurface::Composite->DrawTextA(text2, &surfaceRect, &position, color, 0, printType);

							position.X -= 20;
							printType |= TextPrintType::Right;
							swprintf_s(text2, L"%d", value);
							DSurface::Composite->DrawTextA(text2, &surfaceRect, &position, color, 0, printType);

							position.X += 10;
							printType &= ~TextPrintType::Right;
							printType |= TextPrintType::Center;
							DSurface::Composite->DrawTextA(L"/", &surfaceRect, &position, color, 0, printType);
						}
						else
						{
							DSurface::Composite->DrawTextA(L"-- / --", &surfaceRect, &position, color, 0, printType);
						}
					}

					position.Y += 13;
					{
						int value = -1, maxValue = 0;
						TechnoExt::GetValuesForDisplay(pThis, pTypeExt->BelowSelectedInfoType, value, maxValue);

						if (value >= 0 && maxValue > 0)
						{
							if (pTypeExt->BelowSelectedInfoColor.Get() == ColorStruct{ 0, 0, 0 })
							{
								RulesClass* const pRules = RulesClass::Instance;
								const double ratio = static_cast<double>(value) / maxValue;
								color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
							}
							else
							{
								color = Drawing::RGB_To_Int(pTypeExt->BelowSelectedInfoColor);
							}

							wchar_t text3[0x20] = {0};

							position.X += 10;
							printType &= ~TextPrintType::Center;
							swprintf_s(text3, L"%d", maxValue);
							DSurface::Composite->DrawTextA(text3, &surfaceRect, &position, color, 0, printType);

							position.X -= 20;
							printType |= TextPrintType::Right;
							swprintf_s(text3, L"%d", value);
							DSurface::Composite->DrawTextA(text3, &surfaceRect, &position, color, 0, printType);

							position.X += 10;
							printType &= ~TextPrintType::Right;
							printType |= TextPrintType::Center;
							DSurface::Composite->DrawTextA(L"/", &surfaceRect, &position, color, 0, printType);
						}
						else
						{
							DSurface::Composite->DrawTextA(L"-- / --", &surfaceRect, &position, color, 0, printType);
						}
					}

					if (CameoPCX)
					{
						PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
					}
					else if (pSHP)
					{
						SHPReference* const pCameoRef = pSHP->AsReference();

						char pFilename[0x20];
						strcpy_s(pFilename, RulesExt::Global()->MissingCameo.data());
						_strlwr_s(pFilename);

						if (!_stricmp(pCameoRef->Filename, GameStrings::XXICON_SHP) && strstr(pFilename, ".pcx"))
						{
							PCX::Instance->LoadFile(pFilename);

							if (BSurface* const MissingCameoPCX = PCX::Instance->GetSurface(pFilename))
								PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
						}
						else
						{
							position -= Point2D { 120, 31 };
							surfaceRect.Width = 60;

							DSurface::Composite->DrawSHP(pPal, pSHP, 0, &position, &surfaceRect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
						}
					}

					return;
				}
			}
		}
	}
}

void TacticalButtonsClass::SelectedSwitch()
{
	Phobos::Config::SelectedDisplay_Enable = !Phobos::Config::SelectedDisplay_Enable;

	MessageListClass::Instance->PrintMessage
	(
		(Phobos::Config::SelectedDisplay_Enable ?
			GeneralUtils::LoadStringUnlessMissing("TXT_SELECT_VISIBLE", L"Set select info visible.") :
			GeneralUtils::LoadStringUnlessMissing("TXT_SELECT_INVISIBLE", L"Set select info invisible.")),
		RulesClass::Instance->MessageDelay,
		HouseClass::CurrentPlayer->ColorSchemeIndex,
		true
	);
}

// Hooks

// Mouse trigger hooks
DEFINE_HOOK(0x6931A5, ScrollClass_WindowsProcedure_PressLeftMouseButton, 0x6)
{
	enum { SkipGameCode = 0x6931B4 };

	TacticalButtonsClass* const pButtons = &TacticalButtonsClass::Instance;

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

	TacticalButtonsClass* const pButtons = &TacticalButtonsClass::Instance;

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

	TacticalButtonsClass* const pButtons = &TacticalButtonsClass::Instance;

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

	TacticalButtonsClass* const pButtons = &TacticalButtonsClass::Instance;

	if (pButtons->PressedInButtonsLayer)
	{
		pButtons->PressedInButtonsLayer = false;
		pButtons->PressDesignatedButton(3);

		return SkipGameCode;
	}

	return 0;
}

// Mouse suspend hooks
DEFINE_HOOK(0x692F85, ScrollClass_MouseUpdate_SkipMouseLongPress, 0x7)
{
	enum { CheckMousePress = 0x692F8E, CheckMouseNoPress = 0x692FDC };

	GET(ScrollClass*, pThis, EBX);

	// 555A: AnyMouseButtonDown
	return (pThis->unknown_byte_554A && !TacticalButtonsClass::Instance.PressedInButtonsLayer) ? CheckMousePress : CheckMouseNoPress;
}

DEFINE_HOOK(0x69300B, ScrollClass_MouseUpdate_SkipMouseActionUpdate, 0x6)
{
	enum { SkipGameCode = 0x69301A };

	const Point2D mousePosition = WWMouseClass::Instance->XY1;
	TacticalButtonsClass* const pButtons = &TacticalButtonsClass::Instance;
	pButtons->SetMouseButtonIndex(&mousePosition);

	if (pButtons->MouseIsOverTactical())
		return 0;

	R->Stack(STACK_OFFSET(0x30, -0x24), 0);
	R->EAX(Action::None);
	return SkipGameCode;
}

// Buttons display hooks
DEFINE_HOOK(0x6D4941, TacticalClass_Render_DrawButtonCameo, 0x6)
{
	TacticalButtonsClass* const pButtons = &TacticalButtonsClass::Instance;
	// TODO New buttons (The later draw, the higher layer)

	pButtons->SelectedUpdate();
	pButtons->SelectedDraw();

	pButtons->SWSidebarDraw();

	return 0;
}

// SW buttons update hooks
DEFINE_HOOK(0x4F9283, HouseClass_Update_RecheckTechTree, 0x5)
{
	TacticalButtonsClass::Instance.SWSidebarRecheck();

	return 0;
}

DEFINE_HOOK(0x6A6314, SidebarClass_AddCameo_SupportSWButtons, 0x8)
{
	enum { SkipThisCameo = 0x6A65F5 };

	GET_STACK(const AbstractType, absType, STACK_OFFSET(0x14, 0x4));
	REF_STACK(int, index, STACK_OFFSET(0x14, 0x8));

	return (absType != AbstractType::Special || SuperWeaponTypeClass::Array->Count <= index || TacticalButtonsClass::Instance.SWSidebarAdd(index)) ? 0 : SkipThisCameo;
}

// Extra function hooks
DEFINE_HOOK(0x6AAF46, SelectClass_Action_ButtonClick1, 0x6)
{
	enum { SkipClearMouse = 0x6AB95A };

	GET(const int, index, ESI);

	return TacticalButtonsClass::Instance.SWQuickLaunch(index) ? SkipClearMouse : 0;
}

// SW buttons trigger hooks
DEFINE_HOOK_AGAIN(0x6AAD2F, SelectClass_Action_ButtonClick2, 0x7)
DEFINE_HOOK(0x6AB94F, SelectClass_Action_ButtonClick2, 0xB)
{
	enum { ForceEffective = 0x6AAE7C };

	if (!TacticalButtonsClass::Instance.DummyAction)
		return 0;

	GET(TacticalButtonsClass::DummySelectClass* const , pThis, EDI);

	R->Stack(STACK_OFFSET(0xAC, -0x98), pThis->SWIndex);
	return ForceEffective;
}

DEFINE_HOOK(0x6AB961, SelectClass_Action_ButtonClick3, 0x7)
{
	enum { SkipControlAction = 0x6AB975 };

	TacticalButtonsClass* const pButtons = &TacticalButtonsClass::Instance;

	if (!pButtons->DummyAction)
		return 0;

	pButtons->DummyAction = false;

	return SkipControlAction;
}

// Shortcuts keys hooks
DEFINE_HOOK(0x533E69, UnknownClass_sub_533D20_LoadKeyboardCodeFromINI, 0x6)
{
	GET(CommandClass*, pCommand, ESI);
	GET(int, key, EDI);

	TacticalButtonsClass* const pButtons = &TacticalButtonsClass::Instance;
	const char* name = pCommand->GetName();
	char buffer[29];

	for (int i = 1; i <= 10; ++i)
	{
		sprintf_s(buffer, "SW Sidebar Shortcuts Num %02d", i);

		if (!_strcmpi(name, buffer))
			pButtons->SWSidebarRecord(i, key);
	}

	return 0;
}

DEFINE_HOOK(0x5FB992, UnknownClass_sub_5FB320_SaveKeyboardCodeToINI, 0x6)
{
	GET(CommandClass*, pCommand, ECX);
	GET(int, key, EAX);

	TacticalButtonsClass* const pButtons = &TacticalButtonsClass::Instance;
	const char* name = pCommand->GetName();
	char buffer[30];

	for (int i = 1; i <= 10; ++i)
	{
		sprintf_s(buffer, "SW Sidebar Shortcuts Num %02d", i);

		if (!_strcmpi(name, buffer))
			pButtons->SWSidebarRecord(i, key);
	}

	return 0;
}
