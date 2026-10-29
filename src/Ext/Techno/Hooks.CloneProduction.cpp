#include "Body.h"

#include <Ext/Building/Body.h>
#include <Ext/BuildingType/Body.h>
#include <Helpers/Macro.h>

#pragma region CloneProduction

static void __fastcall KickOutClones(const BuildingExt* const pThis, const TechnoClass* const pProduction)
{
	auto const pFactory = pThis->OwnerObject();
	auto const pFactoryType = pFactory->Type;

	if (pFactoryType->Cloning || (pFactoryType->Factory != InfantryTypeClass::AbsID && pFactoryType->Factory != UnitTypeClass::AbsID))
		return;

	auto pProductionType = pProduction->GetTechnoType();
	auto pProductionTypeExt = TechnoTypeExt::Fetch(pProductionType);

	if (!pProductionTypeExt->Cloneable)
		return;

	if (const auto pClonedAs = pProductionTypeExt->ClonedAs.Get())
	{
		pProductionType = pClonedAs;
		pProductionTypeExt = TechnoTypeExt::Fetch(pProductionType);
	}

	auto const pFactoryOwner = pFactory->Owner;
	auto const& pCloningSources = pProductionTypeExt->ClonedAt;
	auto kickOutClone = [pProductionType, pFactoryOwner](BuildingClass* pBuilding) -> void
	{
		auto const pClone = static_cast<TechnoClass*>(pProductionType->CreateObject(pFactoryOwner));

		if (pBuilding->KickOutUnit(pClone, CellStruct::Empty) != KickOutResult::Succeeded)
			pClone->UnInit();
	};

	auto const isUnit = (pFactoryType->Factory != InfantryTypeClass::AbsID);

	// Keep cloning vats for backward compatibility unless explicit sources are defined.
	if (!isUnit && pCloningSources.empty())
	{
		for (auto const pCloningVat : pFactoryOwner->CloningVats)
			kickOutClone(pCloningVat);
	}

	if (!pCloningSources.empty() || isUnit)
	{
		for (auto const pBuilding : pFactoryOwner->Buildings)
		{
			if (pBuilding->InLimbo)
				continue;

			auto const pBuildingType = pBuilding->Type;
			auto shouldClone = false;

			if (!pCloningSources.empty())
				shouldClone = pCloningSources.Contains(pBuildingType);
			else if (isUnit)
				shouldClone = BuildingTypeExt::Fetch(pBuildingType)->CloningFacility && (pBuildingType->Naval == pFactoryType->Naval);

			if (shouldClone)
				kickOutClone(pBuilding);
		}
	}
}

DEFINE_HOOK(0x4448F8, BuildingClass_KickOutUnit_CloningFacilityFix, 0x6)
{
	GET(const BuildingClass* const, pThis, ESI);
	GET(const UnitClass* const, pUnit, EDI);

	--Unsorted::ScenarioInit;
	KickOutClones(BuildingExt::Fetch(pThis), pUnit);
	++Unsorted::ScenarioInit;

	return 0;
}

#pragma endregion
