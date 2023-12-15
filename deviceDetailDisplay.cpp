#include "deviceDetailDisplay.h"
#include "main.h"
#include "tui.h"
#include <vector>
#include <sstream>
#include <iomanip>

WINDOW *pDeviceDetailsWindow = 0;

enum details {
    detail_option = 0,
    detail_coil,
    detail_di,
    detail_hr,
    detail_ir,
    detail_size
};
//WINDOW* pDetailWindows[detail_size+1] = { 0, 0, 0, 0, 0, 0 };
//menu*   pDetailMenu[detail_size+1]    = { 0, 0, 0, 0, 0, 0 };
menu_state sDetailDisplay[detail_size];

std::list<cModbus_Device>::iterator current_device;

#define edit_in_place(detail, value, field_length, GetStartAddress, SetValue, GetFormattedString)   \
                      edit_in_placeEX(&sDetailDisplay[detail], value, DEFAULT_FIELD_LENGTH, detail, \
                                        GetStartAddress,                                            \
                                        SetValue,                                                   \
                                        GetFormattedString);         
//////////////////
//Cleanup Routines
void cleanDeviceDetailsMenus() {
    menu_state *p = sDetailDisplay;
    for (int i = 0; i != detail_size; i++, p++) {
        if (p->pMenu) {
            delete[] p->pMenu;
            p->pMenu = 0;
        }
    }
}
void cleanDeviceDetailsWindow() {
    menu_state *p = sDetailDisplay;
    for (int i = 0; i != detail_size; i++, p++) {
        if (p->pWindow) {
            delwin(p->pWindow);
            p->pWindow = 0;
        }
    }
}
void paintDeviceDetails() {
    int maxy, maxx;
    //error check.
    if (!_gDeviceListMenuState.pWindow)
        return;
    cleanDeviceDetailsWindow();

    //get max window size for the details window
    maxy = getmaxy(bodywin());
    maxx = getmaxx(bodywin()) - getmaxx(_gDeviceListMenuState.pWindow);
    //make the details window
    if (!pDeviceDetailsWindow) {
        pDeviceDetailsWindow = derwin(bodywin(), maxy, maxx, 0, getmaxx(_gDeviceListMenuState.pWindow));
    }
    //clear the drawing board
    werase(pDeviceDetailsWindow);
    wrefresh(pDeviceDetailsWindow);
    //build the subwindows for the detailed view
    int new_x = 0;
    menu_state *ms = sDetailDisplay;
    for (int i = 0; i != detail_size; i++, ms++) {
        int y, x;
        //get the largest dementions for the menu items.
        menudim(ms->pMenu, &y, &x);
        y += 2; x += 1;
        //do{
           ms->pWindow = derwin(pDeviceDetailsWindow, y, x, 0, new_x);
        //    y--;
        //}while(!pDetailWindows[i]);
        new_x += x; 
        repaintmenu(ms);
    }
}

/// <summary>
/// Receives user input directly from selected menu item
/// </summary>
/// <param name="win">menu window</param>
/// <param name="menu_item">menu item value (argument passed by OnClick()</param>
/// <param name="field_length">Max amount of text to process</param>
/// <param name="GetStartAddress">Start Address of register type, NULL for name</param>
/// <param name="SetValue">Class Method to call to set the desired member</param>
/// <param name="GetFOrmattedString">Class Method to get output string</param>
/// <param name="text_vector">Pointer to the text dump for the menu receiving input</param>
/// <param name="menu_array">Pointer to the menu receiving input.</param>
bool rebuild_details = false;
void edit_in_placeEX(menu_state* ms, uint16_t menu_item, int field_length, int register_type,
    uint16_t    (cModbus_Device::* GetStartAddress)    (void),
    void        (cModbus_Device::* SetValue)           (uint16_t, const char*),
    std::string (cModbus_Device::* GetFormattedString) (uint16_t, const T_Options*)){

    //error check inputs
    if (!ms->pWindow
        ||
        field_length > DEFAULT_FIELD_LENGTH
        ||
        !GetFormattedString) {
        errormsg("edit_in_placeEX() Error");
        return;
    }
    int y, x, c = ERR;
    uint16_t address;
    char buf[DEFAULT_FIELD_LENGTH];

    //Get starting address
    address = 0;
    if( GetStartAddress )
        address = ((*current_device).*GetStartAddress)();
    //correct for the off by one error of the menu item vs item address 
    //due to menu header not being in the Text Dump
    address += menu_item - 1;   

    //copy existing display value into the input buffer
    strncpy(buf,
        ms->pMenu[menu_item].name.c_str(),
        DEFAULT_FIELD_LENGTH);
    //get menu_item position and make a new window over the value to edit
    getbegyx(ms->pWindow, y, x);

    // remove the display prefix and root from the buffer
    {
        char* found = strstr(buf, ": ");                    //search for start of value 'niddle'
        if (found) {                                        //if found
            found += 2;                                     //  shift the 'needle' out of the 'haystack'
            x += found - buf;                               //  move the edge of the edit box the begining of the user data
            strncpy(buf, found, DEFAULT_FIELD_LENGTH);      //  remove the proceding text from the input buffer
        }
    }
    //make the window at the perfect spot (y/x)
    WINDOW* editbox = newwin(1, 6, y + 1 + menu_item, x + 2);

    //launch the edit function in a loop
    do {
        c = weditstr(editbox, buf, field_length);
        //test the return value.
        try {
            if (c == '\n') {
                //save the results
                if (SetValue)
                    ((*current_device).*SetValue)(address, buf);
                //store new formatted text to the menu.name
                ms->pMenu[menu_item].name = ((*current_device).*GetFormattedString)(address, 0);

                //test if the address was conditional and rebuild details if true;
                switch (register_type) {
                case detail_coil:
                    rebuild_details = current_device->IsConditionalAddress(address | TEMPLATE_ADDRESS_COIL);
                    break;
                case detail_di:
                    rebuild_details = current_device->IsConditionalAddress(address | TEMPLATE_ADDRESS_DISCRETE_INPUT); 
                    break;
                case detail_hr:
                    rebuild_details = current_device->IsConditionalAddress(address | TEMPLATE_ADDRESS_HOLDING_REGISTER);
                    break;
                case detail_ir:
                    rebuild_details = current_device->IsConditionalAddress(address | TEMPLATE_ADDRESS_INPUT_REGISTER);
                    break;
                }
                if (rebuild_details) {
                    rebuild_details = false;
                    buildDeviceDetail(current_device->GetAddress());
                    paintDeviceDetails();
                }
            }
            break;  //exit the loop on 1)valid input 2)arrow keys 3)Esc key
        }
        catch (std::invalid_argument const& ex) {
            errormsg(ex.what());    //invalid input, repeat the loop
        }
    } while (1);
    //cleanup
    delwin(editbox);
}

void setname(uint16_t value) {
    edit_in_place(detail_option, value, DEFAULT_FIELD_LENGTH,
                    0,
                    &cModbus_Device::SetName,
                    &cModbus_Device::GetFormattedString_Name);
    BuildDeviceListWindow();
    paintDeviceDetails();
}

void setaddress(uint16_t value) {
    edit_in_place(detail_option, value, 4,
                    0,
                    &cModbus_Device::SetAddress,
                    &cModbus_Device::GetFormattedString_Address);
    BuildDeviceListWindow();
    _gDevice_List.sort();
    key = KEY_ESC;

}
void senddata(uint16_t value) {}
void readdata(uint16_t value) {}
void scanDevice(uint16_t value) {}
void settemplate(uint16_t value) {}

void setcoil(uint16_t value) {
    edit_in_place(detail_coil, value, 4,
        &cModbus_Device::GetStartCoil,
        &cModbus_Device::SetCoil,
        &cModbus_Device::GetFormattedString_Coil);
}
void setHR(uint16_t value) {
    edit_in_place(detail_hr, value, 6,
        &cModbus_Device::GetStartHoldingRegister,
        &cModbus_Device::SetHoldingRegister,
        &cModbus_Device::GetFormattedString_HoldingRegister);
}

void buildDeviceDetail(uint16_t current_selection) {
    cleanDeviceDetailsMenus();
    //find the device to display, exit if we do not find it
    current_device = std::find(_gDevice_List.begin(), _gDevice_List.end(), current_selection);
    if (current_device == _gDevice_List.end())
        return;
    size_t size;
    uint16_t item_number;

    /////////////////////
    /* Start first set */
    /////////////////////
    #define myname sDetailDisplay[detail_option].pMenu
    myname = new menu[8];   //assocate new menu
    myname[7] = END_OF_MENU;//add menu termination
    item_number = 0;
    //Menu Header
    myname[item_number++] = { "Device Options", DoNothing, "", MENU_DISABLE };
    //Name 
    myname[item_number++] = { current_device->GetFormattedString_Name(0), setname, "", item_number};

    //address
    myname[item_number++] = { current_device->GetFormattedString_Address(0), setaddress, "", item_number};
    //the rest of it
    myname[item_number++] = { "Send All Data", senddata, "", item_number};
    myname[item_number++] = { "Recv All Data", readdata, "", item_number};
    myname[item_number++] = { "Scan Device", scanDevice, "Find all registers offered by device", item_number };
    myname[item_number++] = { "Select Template", settemplate, "", current_selection};
    #undef myname
    ////////////////
    /* Second Set */
    //////////////// coils
    #define myname sDetailDisplay[detail_coil].pMenu
    item_number = 0;
    size = current_device->GetCoilN();
    myname = new menu[size + 2];
    myname[size + 1] = END_OF_MENU;
    //menu header
    // menu* breakdown <name> <OnClick> <Description> <menu value> <OnSelect> <text align>
    myname[item_number++] = { "Coils", DoNothing, "", MENU_DISABLE};

    uint16_t a = current_device->GetStartCoil();
    //list of coils
    for (uint16_t i = 0; i != size; i++, a++) {
        myname[item_number++] = { current_device->GetFormattedString_Coil(a), setcoil, "", item_number };
    }
    #undef myname

    ///////////////
    /* Third Set */
    /////////////// Discrete Inputs
    #define myname sDetailDisplay[detail_di].pMenu
    item_number = 0;
    size = current_device->GetDiscreteInputN();
    myname = new menu[size + 2];
    myname[size + 1] = END_OF_MENU;
    //di header
    myname[item_number++] = { "Discrete Inputs", DoNothing, "", MENU_DISABLE};
    a = current_device->GetStartDiscreteInput();
    //list of di
    for (uint16_t i = 0; i != size; i++, a++) {
        myname[item_number++] = { current_device->GetFormattedString_DiscreteInput(a), DoNothing, "", item_number};
    }
    #undef myname

    ///////////////
    /* Forth Set */
    ///////////////holding registers
    #define myname sDetailDisplay[detail_hr].pMenu
    item_number = 0;
    size = current_device->GetHoldingRegisterN();
    myname = new menu[size + 2];
    myname[size + 1] = END_OF_MENU;
    //hr header
    myname[item_number++] = { "Holding Registers", DoNothing, "", MENU_DISABLE, 0};
    a = current_device->GetStartHoldingRegister();
    //list of hr
    for (uint16_t i = 0; i != size; i++, a++) {
        myname[item_number++] = { current_device->GetFormattedString_HoldingRegister(a), setHR, "", item_number};
    }
    #undef myname
    ///////////////
    /* Fifth Set */
    ///////////////input registers
    #define myname sDetailDisplay[detail_ir].pMenu
    item_number = 0;
    size = current_device->GetInputRegisterN();
    myname = new menu[size + 2];
    myname[size + 1] = END_OF_MENU;
    //IR header
    myname[item_number++] = { "Input Registers", DoNothing, "", MENU_DISABLE};
    a = current_device->GetStartInputRegister();
    //list of ir
    for (uint16_t i = 0; i != size; i++, a++) {
        myname[item_number++] = { current_device->GetFormattedString_InputRegister(a), DoNothing, "", item_number};
    }
    #undef myname
}

void handleDeviceDetail() {
    int selection = 0, c = ERR, old = -1;
    bool stop = false;
    while (!stop)
    {
        if (old != selection) {
            paintDeviceDetails();
            old = selection;
        }
        exMenu(&sDetailDisplay[selection], 0, 0); //run device submenu
        switch (key) {
            case KEY_RIGHT:
                if ( selection < detail_size-1 )
                    selection++;
                key = ERR;
                break;
            
            case KEY_ESC:
            case KEY_LEFT:
                if (selection) {
                    selection--;
                }else{
                    stop = true;
                    break;
                }
                key = ERR;
                break;
        }
    }
}