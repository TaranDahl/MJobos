#pragma once
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

// #include <New/Type/AttachEffectTypeClass.h>
// #include <New/Type/ShieldTypeClass.h>
// #include <New/Type/LaserTrailTypeClass.h>
// #include <New/Type/DigitalDisplayTypeClass.h>
// #include <New/Type/SelectBoxTypeClass.h>
// #include <New/Type/Affiliated/InterceptorTypeClass.h>
// #include <New/Type/Affiliated/PassengerDeletionTypeClass.h>
// #include <New/Type/Affiliated/DroppodTypeClass.h>
// #include <New/Type/Affiliated/TiberiumEaterTypeClass.h>
// #include <New/Type/Affiliated/CreateUnitTypeClass.h>

class Matrix3D;
class ParticleSystemTypeClass;
class TechnoTypeExt
{
public:
	using base_type = TechnoTypeClass;

	static constexpr DWORD Canary = 0x11111111;

	class ExtData final : public Extension<TechnoTypeClass>
	{
	public:


		ExtData(TechnoTypeClass* OwnerObject) : Extension<TechnoTypeClass>(OwnerObject)

		{ }

		virtual ~ExtData() = default;
		virtual void LoadFromINIFile(CCINIClass* pINI) override;
		virtual void Initialize() override { }

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		void UpdateAdditionalAttributes();
	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TechnoTypeExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
