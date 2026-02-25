#pragma once

#include <AnimClass.h>
#include <Ext/AnimType/Body.h>

class AnimExt
{
public:
	using base_type = AnimClass;

	static constexpr DWORD Canary = 0xAAAAAAAA;
	static constexpr bool ShouldConsiderInvalidatePointer = false; // Sheer volume of animations in an average game makes a bespoke solution for pointer invalidation worthwhile.

	class ExtData final : public Extension<AnimClass>
	{
	public:
		// TODO : extern
		//TechnoClass* Invoker;
		//HouseClass* InvokerHouse;

		ExtData(AnimClass* OwnerObject) : Extension<AnimClass>(OwnerObject)
		{ }

		// TODO : extern
		//void SetInvoker(TechnoClass* pInvoker);
		//void SetInvoker(TechnoClass* pInvoker, HouseClass* pInvokerHouse);

		virtual ~ExtData() override;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void InitializeConstants() override;

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<AnimExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static void Clear() { };

	static ExtContainer ExtMap;

	// TODO : extern
	//static bool SetAnimOwnerHouseKind(AnimClass* pAnim, HouseClass* pInvoker, HouseClass* pVictim, bool defaultToVictimOwner = false, bool defaultToInvokerOwner = false);
	//static HouseClass* GetOwnerHouse(AnimClass* pAnim, HouseClass* pDefaultOwner = nullptr);
	//static void ChangeAnimType(AnimClass* pAnim, AnimTypeClass* pNewType, bool resetLoops, bool restart);
	//static void CreateRandomAnim(const std::vector<AnimTypeClass*>& AnimList, CoordStruct coords, TechnoClass* pTechno = nullptr, HouseClass* pHouse = nullptr, bool invoker = false, bool ownedObject = false);
};
