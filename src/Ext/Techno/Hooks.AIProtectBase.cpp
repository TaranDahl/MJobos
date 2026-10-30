#include <FootClass.h>
#include <TeamClass.h>

#include <Ext/TechnoType/Body.h>

DEFINE_HOOK(0x70821F, TechnoClass_BaseIsAttacked_Ignore1, 0x6)
{
	enum { CheckDefend = 0x70822B, SkipDefend = 0x7083BC };

	GET(TeamClass*, pTeam, EAX);
	GET(bool, isBaseDefense, ECX);
	GET(FootClass*, pThis, ESI);

	if (isBaseDefense)
		return CheckDefend;

	if (TechnoTypeExt::Fetch(pThis->GetTechnoType())->AIDefendBase_Ignore)
		return SkipDefend;

	return pTeam ? SkipDefend : CheckDefend;
}

DEFINE_HOOK(0x708455, TechnoClass_BaseIsAttacked_Ignore2, 0x6)
{
	enum { CheckDefend = 0x708461, SkipDefend = 0x708622 };

	GET(TeamClass*, pTeam, EAX);
	GET(bool, isBaseDefense, ECX);
	GET(FootClass*, pThis, ESI);

	if (isBaseDefense)
		return CheckDefend;

	if (TechnoTypeExt::Fetch(pThis->GetTechnoType())->AIDefendBase_Ignore)
		return SkipDefend;

	return pTeam ? SkipDefend : CheckDefend;
}
