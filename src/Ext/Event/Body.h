#pragma once
#include <EventClass.h>
#include <TargetClass.h>
#include <HouseClass.h>
#include <TechnoClass.h>

#include <cstddef>
#include <stdint.h>

enum class EventTypeExt : uint8_t
{
	// Vanilla game used Events from 0x00 to 0x2F
	// CnCNet reserved Events from 0x30 to 0x3F
	// Ares used Events 0x60 and 0x61
	// Phobos uses Events from 0x80 onward

	ApproachObject = 0x80,
	TogglePlayerAutoRepair = 0x81,
	ManualReload = 0x82,
	ToggleReversingStance = 0x85,

	FIRST = ApproachObject,
	LAST = ToggleReversingStance
};

#pragma pack(push, 1)
class EventExt
{
public:
	EventTypeExt Type;
	bool IsExecuted;
	char HouseIndex;
	uint32_t Frame;
	union
	{
		char DataBuffer[104];

		struct APPROACHOBJECT
		{
			TargetClass Whom;
			TargetClass Target;
		} ApproachObject;
		struct TogglePlayerAutoRepair
		{ } TogglePlayerAutoRepair;
		struct MANUALRELOAD
		{
			TargetClass Whom;
		} ManualReloadEvent;
		struct ToggleReversingStance
		{
			TargetClass Whom;
		} ToggleReversingStance;
	};

	bool AddEvent();
	void RespondEvent();

	void RespondApproachObject();
	static void RaiseTogglePlayerAutoRepair();
	void RespondToTogglePlayerAutoRepair();

	static void RaiseManualReloadEvent(TechnoClass* pTechno);
	void RespondToManualReloadEvent();
	static void RaiseToggleReversingStance(TechnoClass* pTechno);
	void RespondToToggleReversingStance();

	static size_t GetDataSize(EventTypeExt type);
	static bool IsValidType(EventTypeExt type);
};

static_assert(sizeof(EventExt) == 111);
static_assert(offsetof(EventExt, DataBuffer) == 7);
#pragma pack(pop)

