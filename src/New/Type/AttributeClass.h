#pragma once

#include <string>
#include <vector>
#include <unordered_map>

class CCINIClass;
class INI_EX;
class PhobosStreamReader;
class PhobosStreamWriter;

class AttributeClass final
{
public:
	/// @brief Gets or creates an index for the given attribute name.
	/// @param name The attribute name (case-insensitive).
	/// @return The index of the attribute. Returns -1 if name is null or empty.
	static int GetOrCreateIndex(const char* name);

	/// @brief Gets the name of an attribute by its index.
	/// @param index The attribute index.
	/// @return The attribute name, or nullptr if the index is invalid.
	static const char* GetName(int index);

	/// @brief Gets the total number of registered attributes.
	static int Count();

	/// @brief Parses "&lt;prefix&gt;&lt;N&gt;=" tags (e.g. AttributeList0, ImmuneToAttribute1) from an INI section and populates a sorted index vector.
	/// @param pINI The INI file.
	/// @param pSection The section name.
	/// @param tagNamePrefix The numbered tag name prefix (without the trailing index).
	/// @param outAttributes The output vector to populate with sorted attribute indices.
	static void ParseFromINI(CCINIClass* pINI, const char* pSection, const char* tagNamePrefix, std::vector<int>& outAttributes);

	/// @brief Parses AttributeListX= tags from an INI section and populates a sorted index vector.
	/// @param pINI The INI file.
	/// @param pSection The section name.
	/// @param outAttributes The output vector to populate with sorted attribute indices.
	static void ParseFromINI(CCINIClass* pINI, const char* pSection, std::vector<int>& outAttributes)
	{
		ParseFromINI(pINI, pSection, "AttributeList", outAttributes);
	}

	/// @brief Clears all registered attributes.
	static void Clear();

	/// @brief Saves the attribute registry to a stream.
	static bool SaveGlobals(PhobosStreamWriter& Stm);

	/// @brief Loads the attribute registry from a stream.
	static bool LoadGlobals(PhobosStreamReader& Stm);

private:
	static std::vector<std::string> Names;
	static std::unordered_map<std::string, int> NameToIndex;
};
