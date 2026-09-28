#include "Attribute.h"
#include <New/Type/AttributeClass.h>

DEFINE_EXPORT(HRESULT, Attribute_GetCount_Phobos, int* pCount)
{
	if (!pCount)
		return E_POINTER;

	*pCount = AttributeClass::Count();
	return S_OK;
}
