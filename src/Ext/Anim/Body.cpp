#include "Body.h"

AnimExt::ExtContainer AnimExt::ExtMap;

AnimExt::ExtData::~ExtData()
{
}

// =============================
// load / save

template <typename T>
void AnimExt::ExtData::Serialize(T& Stm)
{
	Stm
		;
}

void AnimExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<AnimClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void AnimExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<AnimClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

void AnimExt::ExtData::InitializeConstants()
{
}

// =============================
// container

AnimExt::ExtContainer::ExtContainer() : Container("AnimClass") { }
AnimExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

namespace CTORTemp
{
	CoordStruct coords;
	unsigned int callerAddress;
}

DEFINE_HOOK(0x421EA0, AnimClass_CTOR_SetContext, 0x6)
{
	GET_STACK(CoordStruct const* const, coords, 0x8);
	GET_STACK(const unsigned int, callerAddress, 0x0);

	CTORTemp::coords = *coords;
	CTORTemp::callerAddress = callerAddress;

	return 0;
}

DEFINE_HOOK(0x422126, AnimClass_CTOR_NullType, 0x5)
{
	Debug::Log("Attempting to create animation with null Type (Caller: %08x)!\n", CTORTemp::callerAddress);
	return 0;
}

DEFINE_HOOK(0x4228D2, AnimClass_CTOR_Load, 0x5)
{
	GET(AnimClass*, pItem, ESI);

	AnimExt::ExtMap.Allocate(pItem);

	return 0;
}

DEFINE_HOOK(0x4226F6, AnimClass_CTOR, 0x6)
{
	GET(AnimClass*, pItem, ESI);

	AnimExt::ExtMap.Allocate(pItem);

	return 0;
}

DEFINE_HOOK(0x422967, AnimClass_DTOR, 0x6)
{
	GET(AnimClass*, pItem, ESI);

	AnimExt::ExtMap.Remove(pItem);

	return 0;
}

/*Crash when Anim called with GameDelete()
DEFINE_HOOK(0x426598, AnimClass_SDDTOR, 0x7)
{
	GET(AnimClass*, pItem, ESI);

	if(AnimExt::ExtMap.Find(pItem))
	AnimExt::ExtMap.Remove(pItem);

	return 0;
}
*/

DEFINE_HOOK_AGAIN(0x425280, AnimClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x4253B0, AnimClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(AnimClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	AnimExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK_AGAIN(0x425391, AnimClass_Load_Suffix, 0x7)
DEFINE_HOOK_AGAIN(0x4253A2, AnimClass_Load_Suffix, 0x7)
DEFINE_HOOK(0x425358, AnimClass_Load_Suffix, 0x7)
{
	AnimExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x4253FF, AnimClass_Save_Suffix, 0x5)
{
	AnimExt::ExtMap.SaveStatic();
	return 0;
}
