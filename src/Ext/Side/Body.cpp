#include "Body.h"

SideExt::ExtContainer SideExt::ExtMap;

void SideExt::ExtData::Initialize()
{
	//const char* pID = this->OwnerObject()->ID;

};

void SideExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	//auto pThis = this->OwnerObject();
	//const char* pSection = pThis->ID;

	//if (!pINI->GetSection(pSection))
	//	return;

	//INI_EX exINI(pINI);
}

// =============================
// load / save

template <typename T>
void SideExt::ExtData::Serialize(T& Stm)
{
	Stm
		;
}

void SideExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<SideClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void SideExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<SideClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool SideExt::LoadGlobals(PhobosStreamReader& Stm)
{
	return Stm.Success();
}

bool SideExt::SaveGlobals(PhobosStreamWriter& Stm)
{
	return Stm.Success();
}

// =============================
// container

SideExt::ExtContainer::ExtContainer() : Container("SideClass") { }
SideExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x6A4609, SideClass_CTOR, 0x7)
{
	GET(SideClass*, pItem, ESI);

	SideExt::ExtMap.TryAllocate(pItem);

	return 0;
}

DEFINE_HOOK(0x6A499F, SideClass_SDDTOR, 0x6)
{
	GET(SideClass*, pItem, ESI);

	SideExt::ExtMap.Remove(pItem);

	return 0;
}

DEFINE_HOOK_AGAIN(0x6A48A0, SideClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x6A4780, SideClass_SaveLoad_Prefix, 0x6)
{
	GET_STACK(SideClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	SideExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x6A488B, SideClass_Load_Suffix, 0x6)
{
	SideExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x6A48FC, SideClass_Save_Suffix, 0x5)
{
	SideExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK(0x679A10, SideClass_LoadAllFromINI, 0x5)
{
	GET_STACK(CCINIClass*, pINI, 0x4);

	for (auto const pSide : SideClass::Array)
		SideExt::ExtMap.Find(pSide)->LoadFromINI(pINI);

	return 0;
}

/*
FINE_HOOK(6725C4, RulesClass_Addition_Sides, 8)
{
	GET(SideClass *, pItem, EBP);
	GET_STACK(CCINIClass*, pINI, 0x38);

	SideExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}
*/
