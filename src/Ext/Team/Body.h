#pragma once
#include <TeamClass.h>

#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

class TeamExt
{
public:
	using base_type = TeamClass;

	static constexpr DWORD Canary = 0x414B4B41;
	static constexpr bool ShouldConsiderInvalidatePointer = false;

	class ExtData final : public Extension<TeamClass>
	{
	public:
		ExtData(TeamClass* OwnerObject) : Extension<TeamClass>(OwnerObject)
		{ }

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override;

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TeamExt>
	{
	public:
		ExtContainer();
		~ExtContainer();

		virtual bool InvalidateExtDataIgnorable(void* const ptr) const override
		{
			return true;
		}
	};

	static ExtContainer ExtMap;

};
