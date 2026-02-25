#pragma once

#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

class BuildingTypeExt
{
public:
	using base_type = BuildingTypeClass;

	static constexpr DWORD Canary = 0x11111111;

	class ExtData final : public Extension<BuildingTypeClass>
	{
	public:

		ExtData(BuildingTypeClass* OwnerObject) : Extension<BuildingTypeClass>(OwnerObject)
		{ }

		// TODO : extern
		// Ares 0.A functions
		//int GetSuperWeaponCount() const;
		//int GetSuperWeaponIndex(int index, HouseClass* pHouse) const;
		//int GetSuperWeaponIndex(int index) const;

		virtual ~ExtData() = default;

		virtual void LoadFromINIFile(CCINIClass* pINI) override;
		virtual void Initialize() override;
		virtual void CompleteInitialization();

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<BuildingTypeExt>
	{
	public:
		ExtContainer();
		~ExtContainer();

		virtual bool Load(BuildingTypeClass* pThis, IStream* pStm) override;
	};

	static ExtContainer ExtMap;
	static bool LoadGlobals(PhobosStreamReader& Stm);
	static bool SaveGlobals(PhobosStreamWriter& Stm);

	// TODO : extern
	//static bool CanUpgrade(BuildingClass* pBuilding, BuildingTypeClass* pUpgradeType, HouseClass* pUpgradeOwner);
	//static int CountOwnedNowWithDeployOrUpgrade(BuildingTypeClass* pBuilding, HouseClass* pHouse);
	//static int GetUpgradesAmount(BuildingTypeClass* pBuilding, HouseClass* pHouse);
};
