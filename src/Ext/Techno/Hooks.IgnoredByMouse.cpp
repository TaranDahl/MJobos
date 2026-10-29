#include <OverlayClass.h>
#include <TerrainClass.h>

#include <Ext/Techno/Body.h>
#include <Ext/OverlayType/Body.h>
#include <Ext/TerrainType/Body.h>
#include <Utilities/Helpers.Alex.h>

static inline bool ShouldIgnoreByMouse(ObjectClass* pObject)
{
	const auto pType = pObject->GetType();

	if (!pType)
		return true;

	if (const auto pTechno = abstract_cast<TechnoClass*, true>(pObject))
	{
		const auto pTypeExt = TechnoExt::Fetch(pTechno)->TypeExtData;
		const auto pOwner = pTechno->Owner;
		const auto defaultValue = pTypeExt->IgnoredByMouse;

		if (pOwner == HouseClass::CurrentPlayer)
			return pTypeExt->IgnoredByMouse_ToSelf.Get(defaultValue);
		else if (pOwner->IsAlliedWith(HouseClass::CurrentPlayer))
			return pTypeExt->IgnoredByMouse_ToAlly.Get(defaultValue);

		return pTypeExt->IgnoredByMouse_ToEnemy.Get(defaultValue);
	}

	const auto absType = pObject->WhatAmI();

	if (absType == OverlayClass::AbsID)
	{
		const auto pOverlay = static_cast<OverlayClass*>(pObject);
		const auto pTypeExt = OverlayTypeExt::Fetch(pOverlay->Type);
		return pTypeExt->IgnoredByMouse.Get();
	}
	else if (absType == TerrainClass::AbsID)
	{
		const auto pTerrain = static_cast<TerrainClass*>(pObject);
		const auto pTypeExt = TerrainTypeExt::Fetch(pTerrain->Type);
		return pTypeExt->IgnoredByMouse.Get();
	}

	return false;
}

DEFINE_HOOK(0x6DA3D2, TacticalClass_GetObjectOnCrd_IgnoredByMouse1, 0x8)
{
	enum { Ignore = 0x6DA491, DontIgnore = 0x6DA3DA };

	GET(ObjectClass* const, pObject, ESI);

	return !pObject || ShouldIgnoreByMouse(pObject) ? Ignore : DontIgnore;
}

// Attachment owns the same address for TransparentToMouse and is composed in Phase F.
DEFINE_HOOK(0x6DA4FB, TacticalClass_GetObjectOnCrd_IgnoredByMouse2, 0x6)
{
	enum { SkipGameCode = 0x6DA501 };

	GET(CellClass* const, pCell, EAX);

	ObjectClass* pFoundObject = nullptr;

	for (auto pOccupier = pCell->FirstObject; pOccupier; pOccupier = pOccupier->NextObject)
	{
		if (ShouldIgnoreByMouse(pOccupier))
			continue;

		pFoundObject = pOccupier;
		break;
	}

	R->EAX(pFoundObject);
	return SkipGameCode;
}
