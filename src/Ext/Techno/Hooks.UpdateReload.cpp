#include <Ext/Rules/Body.h>
#include <Ext/TechnoType/Body.h>
#include <Utilities/AresHelper.h>

DEFINE_HOOK(0x51BDCF, InfantryClass_Update_Reload, 0x7)
{
	enum { SkipGameCode = 0x51BDD6 };

	GET(InfantryClass*, pThis, ESI);

	R->EAX(pThis->InWhichLayer());

	if (RulesExt::Global()->InTransportInfantryAmmoFix
		&& AresHelper::CanUseAres
		&& pThis->InLimbo
		&& !TechnoTypeExt::Fetch(pThis->Type)->ReloadInTransport)
	{
		pThis->Reload();
	}

	return SkipGameCode;
}
