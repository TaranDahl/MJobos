#pragma once
#include <SideClass.h>

#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

class SideExt
{
public:
	using base_type = SideClass;

	static constexpr DWORD Canary = 0x05B10501;

	class ExtData final : public Extension<SideClass>
	{
	public:
		ExtData(SideClass* OwnerObject) : Extension<SideClass>(OwnerObject)
		{ }

		virtual ~ExtData() = default;

		virtual void LoadFromINIFile(CCINIClass* pINI) override;
		virtual void Initialize() override;
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<SideExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
	static bool LoadGlobals(PhobosStreamReader& Stm);
	static bool SaveGlobals(PhobosStreamWriter& Stm);
};
