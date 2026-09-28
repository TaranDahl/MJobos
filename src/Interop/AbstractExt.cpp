#include "AbstractExt.h"
#include <Utilities/Container.h>
#include <New/Type/AttributeClass.h>
#include <algorithm>

DEFINE_EXPORT(HRESULT, Abstract_GetAttributeIndices_Phobos, AbstractClass* pThis, int* pOutIndices, int maxCount, int* pCount)
{
	if (!pThis || !pCount)
		return E_POINTER;

	auto const pExt = AbstractExt::Fetch(pThis);

	if (!pExt)
		return E_FAIL;

	const auto& attributes = pExt->GetAttributes();
	*pCount = static_cast<int>(attributes.size());

	if (!pOutIndices || maxCount <= 0)
		return S_OK;

	const int count = std::min<int>(maxCount, *pCount);

	for (int i = 0; i < count; ++i)
		pOutIndices[i] = attributes[i];

	return S_OK;
}

DEFINE_EXPORT(HRESULT, Abstract_GetAttributeNames_Phobos, AbstractClass* pThis, const char** pOutNames, int maxCount, int* pCount)
{
	if (!pThis || !pCount)
		return E_POINTER;

	auto const pExt = AbstractExt::Fetch(pThis);

	if (!pExt)
		return E_FAIL;

	const auto& attributes = pExt->GetAttributes();
	*pCount = static_cast<int>(attributes.size());

	if (!pOutNames || maxCount <= 0)
		return S_OK;

	const int count = std::min<int>(maxCount, *pCount);

	for (int i = 0; i < count; ++i)
		pOutNames[i] = AttributeClass::GetName(attributes[i]);

	return S_OK;
}
