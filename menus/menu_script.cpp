#include "menu_devices.h"
#include "../tui.h"
#include <stdlib.h>

void AddDevice(const uint16_t value);
void Menu_Callback_Script(const uint16_t value)
{
    menu SubMenuScript[] =
    {
        { "Load", NotYetImplemented, "Load script from file"},
        { "Run",  NotYetImplemented, "Run loaded script"},
        { "Stop", NotYetImplemented, "Stop running script"},
        END_OF_MENU
    };

    menu_state SubMenuScriptState = { SubMenuScript, 0, 0, 1 };
    SubMenuScriptState.hotkey = true;
    domenu(&SubMenuScriptState);
}