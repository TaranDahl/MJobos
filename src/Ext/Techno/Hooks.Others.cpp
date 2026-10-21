#include <Ext/Rules/Body.h>
#include <Ext/Techno/Body.h>

#include <BuildingClass.h>
#include <InfantryClass.h>
#include <ParasiteClass.h>
#include <Utilities/Macro.h>

DEFINE_HOOK(0x522937, InfantryClass_EnterOccupyBuilding_KeepUpdate, 0xA)
{
	GET(InfantryClass*, pThis, ESI);
	GET_STACK(BuildingClass*, pBuilding, STACK_OFFSET(0x1C, 0x4));

	if (RulesExt::Global()->UpdateInLimbo_Occupier)
		LogicClass::Instance.AddObject(pThis, false);

	TechnoExt::Fetch(pThis)->BuildingOccupying = pBuilding;

	return 0;
}

DEFINE_HOOK(0x4733B6, PassengerClass_AddPassenger_KeepUpdate, 0x5)
{
	GET(InfantryClass*, pThis, ESI);

	if (RulesExt::Global()->UpdateInLimbo_NormalPassenger)
		LogicClass::Instance.AddObject(pThis, false);

	return 0;
}

DEFINE_HOOK(0x62A0A0, ParasiteClass_Update_UpdateParasiteLocation, 0x7)
{
	GET(ParasiteClass*, pThis, ESI);

	if (RulesExt::Global()->UpdateInLimbo_LimboLaunch)
		pThis->Owner->SetLocation(pThis->Victim->Location);

	return 0;
}

DEFINE_HOOK(0x6FF7F9, SITechnoClass_Fire_LimboLaunch, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	if (RulesExt::Global()->UpdateInLimbo_LimboLaunch)
		LogicClass::Instance.AddObject(pThis, false);

	return 0;
}

DEFINE_HOOK(0x4580B4, BuildingClass_OccupantLeaveAll_UpdateState, 0x5)
{
	GET(InfantryClass*, pOccupant, EDI);

	TechnoExt::Fetch(pOccupant)->BuildingOccupying = nullptr;

	return 0;
}

DEFINE_HOOK(0x6FC5B3, TechnoClass_GetFireError_InLimbo, 0x6)
{
	enum { Illegal = 0x6FC86A };

	GET(TechnoClass*, pThis, ESI);

	return !pThis->InLimbo || pThis->InOpenToppedTransport
		|| (RulesExt::Global()->UpdateInLimbo_Occupier && TechnoExt::Fetch(pThis)->BuildingOccupying)
		? 0 : Illegal;
}
