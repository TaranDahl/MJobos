#include "AttributeClass.h"

#include <CCINIClass.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/SavegameDef.h>

#include <algorithm>
#include <cctype>
#include <string>

std::vector<std::string> AttributeClass::Names;
std::unordered_map<std::string, int> AttributeClass::NameToIndex;

int AttributeClass::GetOrCreateIndex(const char* name)
{
	if (!name || !*name)
		return -1;

	std::string key(name);
	// Convert to lowercase for case-insensitive matching
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);

	auto it = NameToIndex.find(key);

	if (it != NameToIndex.end())
		return it->second;

	int index = static_cast<int>(Names.size());
	Names.push_back(key);
	NameToIndex[key] = index;
	return index;
}

const char* AttributeClass::GetName(int index)
{
	if (index < 0 || index >= static_cast<int>(Names.size()))
		return nullptr;

	return Names[index].c_str();
}

int AttributeClass::Count()
{
	return static_cast<int>(Names.size());
}

void AttributeClass::ParseFromINI(CCINIClass* pINI, const char* pSection, const char* tagNamePrefix, std::vector<int>& outAttributes)
{
	outAttributes.clear();

	if (!pINI || !pSection || !*pSection || !tagNamePrefix || !*tagNamePrefix)
		return;

	if (!pINI->GetSection(pSection))
		return;

	INI_EX exINI(pINI);

	for (size_t i = 0;; ++i)
	{
		char tagName[0x60];
		_snprintf_s(tagName, _TRUNCATE, "%s%zu", tagNamePrefix, i);

		if (exINI.ReadString(pSection, tagName) <= 0)
			break;

		// Split the value by comma and register each attribute
		const char* str = Phobos::readBuffer;
		const char* start = str;

		while (*start)
		{
			// Skip leading whitespace
			while (*start && (*start == ' ' || *start == '\t'))
				++start;

			if (!*start)
				break;

			const char* end = start;

			// Find the end of this token (comma or end of string)
			while (*end && *end != ',')
				++end;

			// Trim trailing whitespace
			const char* trimEnd = end;

			while (trimEnd > start && (*(trimEnd - 1) == ' ' || *(trimEnd - 1) == '\t'))
				--trimEnd;

			if (trimEnd > start)
			{
				std::string attrName(start, trimEnd - start);
				int index = GetOrCreateIndex(attrName.c_str());

				if (index >= 0)
					outAttributes.push_back(index);
			}

			start = *end ? end + 1 : end;
		}
	}

	// Sort the indices for binary search
	std::sort(outAttributes.begin(), outAttributes.end());
}

void AttributeClass::Clear()
{
	Names.clear();
	NameToIndex.clear();
}

bool AttributeClass::SaveGlobals(PhobosStreamWriter& Stm)
{
	Stm.Save(Names.size());

	for (const auto& name : Names)
		Savegame::WritePhobosStream(Stm, name);

	return true;
}

bool AttributeClass::LoadGlobals(PhobosStreamReader& Stm)
{
	Clear();

	size_t count = 0;

	if (!Stm.Load(count))
		return false;

	Names.reserve(count);

	for (size_t i = 0; i < count; ++i)
	{
		std::string name;

		if (!Savegame::ReadPhobosStream(Stm, name, false))
			return false;

		Names.push_back(name);
		NameToIndex[name] = static_cast<int>(i);
	}

	return true;
}
