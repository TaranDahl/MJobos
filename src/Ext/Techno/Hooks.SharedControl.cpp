#include <HouseClass.h>

#include <Ext/Rules/Body.h>

DEFINE_HOOK(0x50B716, HouseClass_IsCurrentPlayer_SharedControl, 0x6)
{
	GET(HouseClass*, pThis, ECX);

	R->EAX(RulesExt::Global()->AllyShareControl
		? pThis->IsAlliedWith(HouseClass::CurrentPlayer)
		: pThis == HouseClass::CurrentPlayer);

	return 0x50B723;
}
