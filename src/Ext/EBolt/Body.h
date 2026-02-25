#pragma once
#include <EBolt.h>

#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

class EBoltExt
{
public:
	using base_type = EBolt;

	static constexpr DWORD Canary = 0x06C28114;

	class ExtData final : public Extension<EBolt>
	{
	public:
		ExtData(EBolt* OwnerObject) : Extension<EBolt>(OwnerObject)
		{ }

		virtual ~ExtData() = default;

		virtual void Initialize() override { };

		virtual void InvalidatePointer(void* ptr, bool removed) override;

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<EBoltExt>
	{
	public:
		ExtContainer();
		~ExtContainer();

		virtual bool InvalidateExtDataIgnorable(void* const ptr) const override;
	};

	static ExtContainer ExtMap;

	static bool LoadGlobals(PhobosStreamReader& Stm);
	static bool SaveGlobals(PhobosStreamWriter& Stm);

};
