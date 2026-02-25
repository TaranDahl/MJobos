#include "Commands.h"

#include <Utilities/TemplateDef.h>

#include <CCINIClass.h>

DEFINE_HOOK(0x533066, CommandClassCallback_Register, 0x6)
{
	// Load it after Ares'

	//MakeCommand<NextIdleHarvesterCommandClass>();

	return 0;
}

static void MouseWheelDownCommand()
{
	//if (MessageColumnClass::Instance.IsHovering())
	//	MessageColumnClass::Instance.ScrollDown();
}

static void MouseWheelUpCommand()
{
	//if (MessageColumnClass::Instance.IsHovering())
	//	MessageColumnClass::Instance.ScrollUp();
}

DEFINE_HOOK(0x777998, Game_WndProc_ScrollMouseWheel, 0x6)
{
	GET(const WPARAM, WParam, ECX);

	if (WParam & 0x80000000u)
		MouseWheelDownCommand();
	else
		MouseWheelUpCommand();

	return 0;
}

static inline bool CheckSkipScrollSidebar()
{
	//if (MessageColumnClass::Instance.IsHovering())
	//	return true;

	return false;
}

DEFINE_HOOK(0x533F50, Game_ScrollSidebar_Skip, 0x5)
{
	enum { SkipScrollSidebar = 0x533FC3 };
	return CheckSkipScrollSidebar() ? SkipScrollSidebar : 0;
}
