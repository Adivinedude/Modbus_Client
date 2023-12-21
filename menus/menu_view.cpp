#include "menu_devices.h"
#include "../tui.h"
#include <stdlib.h>

void AddDevice(const uint16_t value);
void Menu_Callback_View(const uint16_t value)
{
    menu SubMenuView[] =
    {
        { "Detail View", NotYetImplemented, "View detailed information"},
        { "Summary View", NotYetImplemented, "Show limited information"},
        END_OF_MENU
    };

    menu_state SubMenuViewState = { SubMenuView, 0,0,1 };
    SubMenuViewState.hotkey = true;
    domenu(&SubMenuViewState);
}