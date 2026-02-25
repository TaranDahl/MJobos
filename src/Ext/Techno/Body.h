#pragma once

#include <Ext/TechnoType/Body.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>
// #include <New/Entity/ShieldClass.h>
// #include <New/Entity/LaserTrailClass.h>
// #include <New/Entity/AttachEffectClass.h>

class BulletClass;

class TechnoExt
{
public:
	using base_type = TechnoClass;

	static constexpr DWORD Canary = 0x55555555;
	static constexpr bool ShouldConsiderInvalidatePointer = false;

	class ExtData final : public Extension<TechnoClass>
	{
	public:
		TechnoTypeExt::ExtData* TypeExtData;
		// TODO : extern
		//std::unique_ptr<ShieldClass> Shield;
		//std::vector<std::unique_ptr<AttachEffectClass>> AttachedEffects;
		//AttachEffectTechnoProperties AE;
		//int AnimRefCount; // Used to keep track of how many times this techno is referenced in anims f.ex Invoker, ParentBuilding etc., for pointer invalidation.

		ExtData(TechnoClass* OwnerObject) : Extension<TechnoClass>(OwnerObject)
			, TypeExtData { nullptr }
		{ }

		// TODO : callback
		void OnEarlyUpdate();

		// TODO : extern
		//bool HasAttachedEffects(std::vector<AttachEffectTypeClass*> attachEffectTypes, bool requireAll, bool ignoreSameSource, TechnoClass* pInvoker, AbstractClass* pSource, std::vector<int> const* minCounts, std::vector<int> const* maxCounts) const;
		//int GetAttachedEffectCumulativeCount(AttachEffectTypeClass* pAttachEffectType, bool ignoreSameSource = false, TechnoClass* pInvoker = nullptr, AbstractClass* pSource = nullptr) const;

		virtual ~ExtData() = default;
		virtual void InvalidatePointer(void* ptr, bool bRemoved) { };
		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TechnoExt>
	{
	public:
		ExtContainer();
		~ExtContainer();

	};

	static ExtContainer ExtMap;

	static UnitClass* Deployer;

	static bool LoadGlobals(PhobosStreamReader& Stm);
	static bool SaveGlobals(PhobosStreamWriter& Stm);

	// TODO : extern
	//static bool IsActive(TechnoClass* pThis);
	//static bool IsActiveIgnoreEMP(TechnoClass* pThis);

	//static bool HasAvailableDock(TechnoClass* pThis);
	//static bool HasRadioLinkWithDock(TechnoClass* pThis);

	//static CoordStruct GetFLHAbsoluteCoords(TechnoClass* pThis, CoordStruct flh, bool turretFLH = false);

	//static CoordStruct GetBurstFLH(TechnoClass* pThis, int weaponIndex, bool& FLHFound);
	//static CoordStruct GetSimpleFLH(InfantryClass* pThis, int weaponIndex, bool& FLHFound);

	//static void KillSelf(TechnoClass* pThis, AutoDeathBehavior deathOption, const std::vector<AnimTypeClass*>& pVanishAnimation, bool isInLimbo = false);
	//static void ObjectKilledBy(TechnoClass* pThis, TechnoClass* pKiller);
	//static double GetCurrentSpeedMultiplier(FootClass* pThis);
	//static double GetCurrentFirepowerMultiplier(TechnoClass* pThis);
	//static void SyncInvulnerability(TechnoClass* pFrom, TechnoClass* pTo);
	//static CoordStruct PassengerKickOutLocation(TechnoClass* pThis, FootClass* pPassenger, int maxAttempts);
	//static bool ConvertToType(FootClass* pThis, TechnoTypeClass* toType);
	//static Point2D GetScreenLocation(TechnoClass* pThis);
	//static bool CannotMove(UnitClass* pThis);

	//static bool EjectRandomly(FootClass* pEjectee, const CoordStruct& coords, int distance, bool select);
	//static bool EjectSurvivor(FootClass* pSurvivor, CoordStruct coords, bool select);

	//// WeaponHelpers.cpp
	//static int PickWeaponIndex(TechnoClass* pThis, TechnoClass* pTargetTechno, AbstractClass* pTarget, int weaponIndexOne, int weaponIndexTwo, bool allowFallback = true, bool allowAAFallback = true);
	//static void FireWeaponAtSelf(TechnoClass* pThis, WeaponTypeClass* pWeaponType);
	//static WeaponTypeClass* GetCurrentWeapon(TechnoClass* pThis, TechnoTypeClass* pType, int& weaponIndex, bool getSecondary = false);
	//static WeaponTypeClass* GetCurrentWeapon(TechnoClass* pThis, TechnoTypeClass* pType, bool getSecondary = false);
};
