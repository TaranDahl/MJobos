#include "Body.h"

#include <EventClass.h>
#include <SpawnManagerClass.h>
#include <OverlayClass.h>
#include <TerrainClass.h>
#include <JumpjetLocomotionClass.h>
#include <Kamikaze.h>

#include <Ext/Anim/Body.h>
#include <Ext/Aircraft/Body.h>
#include <Ext/Building/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/OverlayType/Body.h>
#include <Ext/TerrainType/Body.h>
#include <Ext/Scenario/Body.h>
#include <Ext/Unit/Body.h>
#include <Utilities/AresHelper.h>
#include <Utilities/Helpers.Alex.h>
#include <Helpers/Macro.h>

#include <TacticalClass.h>
#include <Commands/FrameByFrame.h>

#pragma region InfantrySquad

static inline InfantryClass* CreateInfantryFromFactory(InfantryTypeClass* pType, HouseClass* pOwner)
{
	// BuildLimit check goes before creation
	if (pType->BuildLimit > 0)
	{
		int sum = pOwner->CountOwnedNow(pType);

		// copy Ares' deployable units x build limit fix
		if (auto const pUndeploy = pType->UndeploysInto)
			sum += pOwner->CountOwnedNow(pUndeploy);

		if (sum >= pType->BuildLimit)
			return nullptr;
	}

	return static_cast<InfantryClass*>(pType->CreateObject(pOwner));
}

DEFINE_HOOK(0x444DDF, BuildingClass_KickOutUnit_InfantrySquad, 0x5)
{
	enum { SkipGameCode = 0x444971 };

	GET(BuildingClass*, pThis, ESI);

	const auto pThisType = pThis->Type;

	if (pThisType->Factory == AbstractType::InfantryType || pThisType->Cloning)
	{
		GET(TechnoClass*, pTechno, EDI);

		if (const auto pInfantry = abstract_cast<InfantryClass*, true>(pTechno))
		{
			const auto pExt = TechnoExt::Fetch(pInfantry);
			const auto pTypeExt = pExt->TypeExtData;
			const int size = pTypeExt->Squad_Members.size();

			if (size > 0)
			{
				if (pTypeExt->Squad_IsInitAsTeam)
				{
					pExt->SquadManager = SquadManagerClass::Allocate();
					pExt->SquadManager->Members.reserve(size + 1);
					pExt->SquadManager->AddMember(pInfantry);
				}

				const auto pOwner = pInfantry->Owner;
				const auto pDestination = (pThis->ArchiveTarget ? pThis->ArchiveTarget : pInfantry);

				for (int i = 0; i < size; ++i)
				{
					if (const auto pMember = CreateInfantryFromFactory(pTypeExt->Squad_Members[i], pOwner))
					{
						++Unsorted::ScenarioInit;
						pMember->Unlimbo(pInfantry->Location, DirType::North);
						pMember->SetDestination(pDestination, true);
						pMember->QueueMission(Mission::Move, 0);
						--Unsorted::ScenarioInit;

						if (pExt->SquadManager)
						{
							TechnoExt::Fetch(pMember)->SquadManager = pExt->SquadManager;
							pExt->SquadManager->AddMember(pMember);
						}
					}
				}
			}
		}
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x6FBFD0, TechnoClass_Select_SquadSelect, 0x5)
{
	GET(TechnoClass*, pTechno, ESI);

	if (pTechno->Owner->IsControlledByCurrentPlayer())
	{
		if (const auto pSquadManager = TechnoExt::Fetch(pTechno)->SquadManager)
		{
			if (!pSquadManager->Selecting)
			{
				pSquadManager->Selecting = true;

				for (auto& pMember : pSquadManager->Members)
				{
					if (pMember != pTechno)
						pMember->Select();
				}

				pSquadManager->Selecting = false;
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x737BBE, UnitClass_Unlimbo_CreatePassengerSquad, 0x6)
{
	GET(UnitClass*, pThis, ESI);

	const auto pExt = TechnoExt::Fetch(pThis);

	if (pExt->TypeExtData->Squad_IsInitAsTeam)
	{
		if (pThis->Passengers.NumPassengers > 0)
		{
			pExt->SquadManager = SquadManagerClass::Allocate();
			pExt->SquadManager->Members.reserve(pThis->Passengers.NumPassengers + 1);
			pExt->SquadManager->AddMember(pThis);

			for (auto pPassenger = pThis->Passengers.GetFirstPassenger(); pPassenger; pPassenger = abstract_cast<FootClass*>(pPassenger->NextObject))
			{
				TechnoExt::Fetch(pPassenger)->SquadManager = pExt->SquadManager;
				pExt->SquadManager->AddMember(pPassenger);
			}
		}
	}

	return 0;
}

#pragma endregion
