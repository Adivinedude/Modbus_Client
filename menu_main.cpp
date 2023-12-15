#include "menu_main.h"
#include "menus/menu_connections.h" 
#include "menus/menu_devices.h"
#include "menus/menu_script.h"
#include "menus/menu_view.h"

/***************************** menus initialization ***********************/
menu MainMenu[] =
{
    { "Connection", Menu_Callback_Connect,  "Connection Settings"},
    { "Devices",    Menu_Callback_Devices,  "Modify Connected Devices"},
    { "Script",     Menu_Callback_Script,   "Load/Exceute Scripts"},
    { "View",       Menu_Callback_View,     "View Settings"},
    END_OF_MENU   /* always add this as the last item! */
};

menu_state MainMenuState = {MainMenu, 0, 0, 0};
