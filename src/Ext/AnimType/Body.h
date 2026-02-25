#pragma once

#include <AnimTypeClass.h>

#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

enum class AttachedAnimPosition : BYTE
{
	Default = 0,
	Center = 1,
	Ground = 2
};

class AnimTypeExt
{
public:
	using base_type = AnimTypeClass;

	static constexpr DWORD Canary = 0xEEEEEEEE;

	class ExtData final : public Extension<AnimTypeClass>
	{
	public:

		ExtData(AnimTypeClass* OwnerObject) : Extension<AnimTypeClass>(OwnerObject)

		{ }

		virtual ~ExtData() = default;

		virtual void LoadFromINIFile(CCINIClass* pINI) override;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<AnimTypeExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

};
