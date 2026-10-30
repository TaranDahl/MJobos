#include "Body.h"

DEFINE_HOOK(0x6F8E44, TechnoClass_SelectAutoTarget_NoAutoFire, 0x7)
{
	enum { SkipAutoTarget = 0x6F8E38 };

	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::Fetch(pThis->GetTechnoType());

	return pTypeExt->NoAutoFire_AI && !pThis->Owner->IsControlledByHuman()
		? SkipAutoTarget
		: 0;
}
