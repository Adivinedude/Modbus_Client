#include "deviceDetailDisplay.h"
#include "main.h"
#include "tui.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include "Modbus_Poller.h"

WINDOW *pDeviceDetailsWindow = 0;

enum details {
    detail_option = 0,
    detail_coil,
    detail_di,
    detail_hr,
    detail_ir,
    detail_size
};

menu_state sDetailList;  //menu object for device list
menu_state sDetailDisplay[detail_size];
std::list<cModbus_Device>::iterator current_device;

void GetTemplateFilename(uint16_t);
void ConfigureDevice(uint16_t);

#define edit_in_place(detail, value, field_length, GetStartAddress, SetValue, GetFormattedString)   \
                      edit_in_placeEX(&sDetailDisplay[detail], value, DEFAULT_FIELD_LENGTH, detail, \
                                        GetStartAddress,                                            \
                                        SetValue,                                                   \
                                        GetFormattedString);         

void SetupBodyWindow()
{
    BodyRedrawStack.push_back(&sDetailList);
    for(int i = 0; i != detail_size; i++)
        BodyRedrawStack.push_back(&sDetailDisplay[i]);
}

void OnSelectDevice(const uint16_t value) {
    buildDeviceDetail(value);
    paintDeviceDetails();
}
void OnClickDevice(const uint16_t value) {

}
void BuildDeviceList() {
    // clear previous menu
    if (sDetailList.pMenu)
        delete[] sDetailList.pMenu;
    // clear the previous window
    if (sDetailList.pWindow)
        delwin(sDetailList.pWindow);

    //make new menu for the device list
    size_t s;
    if (s = _gDevice_List.size()) {
        sDetailList.pMenu = new menu[s + 2];
    }
    else {
        sDetailList.pMenu = new menu[2];
    }
    //Set Draw Options
    sDetailList.alwaysSelected = true;
    sDetailList.alwaysDrawBox = true;
    //add menu header
    sDetailList.pMenu[0] = { "Device List", DoNothing, "", MENU_DISABLE };

    //add each device to the menu
    {
        menu* p = sDetailList.pMenu;
        p++;
        for (std::list<cModbus_Device>::iterator iter = _gDevice_List.begin();
            iter != _gDevice_List.end(); iter++, p++) {
            *p = { iter->GetName(), OnClickDevice, "", iter->GetAddress(), OnSelectDevice };
        }
    }
    //terminate the menu
    sDetailList.pMenu[s + 1] = END_OF_MENU;

    int y, x;
    //Get the menu dimensions
    menudim(sDetailList.pMenu, &y, &x);
    //make new window for the menu
    sDetailList.pWindow = derwin(bodywin(), y + 2, x + 2, 0, 0);
    clsbody();                                      //clear main body window
    colorbox(sDetailList.pWindow, SUBMENUCOLOR, 1);   //set color and outline
    repaintmenu(&sDetailList);  //paint new menu
    touchwin(bodywin());                            //paint body window
    wrefresh(bodywin());
}


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
    if (!sDetailList.pWindow)
        return;
    cleanDeviceDetailsWindow();

    //get max window size for the details window
    maxy = getmaxy(bodywin());
    maxx = getmaxx(bodywin()) - getmaxx(sDetailList.pWindow);
    //make the details window
    if (!pDeviceDetailsWindow) {
        pDeviceDetailsWindow = derwin(bodywin(), maxy, maxx, 0, getmaxx(sDetailList.pWindow));
    }
    //clear the drawing board
    werase(pDeviceDetailsWindow);
    //build the subwindows for the detailed view
    int new_x = 0;
    menu_state *ms = sDetailDisplay;
    for (int i = 0; i != detail_size; i++, ms++) {
        int y, x;
        //get the largest dementions for the menu items.
        menudim(ms->pMenu, &y, &x);
        y += 2; x += 1;
        ms->pWindow = derwin(pDeviceDetailsWindow, (y > maxy)?maxy:y, x, 0, new_x);
        new_x += x; 
        repaintmenu(ms);
    }
    wrefresh(pDeviceDetailsWindow);
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
        ms->pMenu[menu_item].pName->GetValue(),
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
    menu_state editboxmenu(editbox);
    BodyRedrawStack.push_back(&editboxmenu);
    //launch the edit function in a loop
    do {
        c = weditstr(editbox, buf, field_length);
        //test the return value.
        try {
            if (c == '\n') {
                //save the results
                if (SetValue)
                    ((*current_device).*SetValue)(address, buf);
                current_device->force_update();
            }
            break;  //exit the loop on 1)valid input 2)arrow keys 3)Esc key
        }
        catch (std::invalid_argument const& ex) {
            errormsg(ex.what());    //invalid input, repeat the loop
        }
    } while (1);
    //cleanup
    delwin(editbox);
    BodyRedrawStack.pop_back();
}

void setname(uint16_t value) {
    edit_in_place(detail_option, value, DEFAULT_FIELD_LENGTH,
                    0,
                    &cModbus_Device::SetName,
                    &cModbus_Device::GetFormattedString_Name);
    BuildDeviceList();
    paintDeviceDetails();
}
void setaddress(uint16_t value) {
    edit_in_place(detail_option, value, 4,
                    0,
                    &cModbus_Device::SetAddress,
                    &cModbus_Device::GetFormattedString_Address);
    BuildDeviceList();
    _gDevice_List.sort();
    key = KEY_ESC;

}
void scanDevice(uint16_t value) { 
    start_ScanDevice();
    buildDeviceDetail(current_device->GetAddress());
}
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

class menu_item_name : public menu_text {
    public: const char* GetValue(){ 
        return current_device->GetName(); 
    }
};
class menu_item_address : public menu_text {
    std::string content;
    public: const char* GetValue(){ 
        content = current_device->GetFormattedString_Address(0); 
        return content.c_str(); 
    };
};
class mibc : public menu_text { //Menu_Item_Base_Class is just to many keystrokes. 'mibc' is easier to type.
    protected:
        std::string content;
        uint16_t    address;
    public: mibc(uint16_t a):address(a){};
            virtual ~mibc(){};
};
class menu_item_coil : public mibc {
    public:
        menu_item_coil(uint16_t a) : mibc(a) {};
        virtual ~menu_item_coil(){};
        const char* GetValue(){
            //if( current_device->update_coils )
                content = current_device->GetFormattedString_Coil(address); 
            return content.c_str(); 
        }
};
class menu_item_discrete_input : public mibc {
    public:
        menu_item_discrete_input(uint16_t a) : mibc(a) {};
        virtual ~menu_item_discrete_input(){};
        const char* GetValue(){
            //if( current_device->update_discrete_input)
                content = current_device->GetFormattedString_DiscreteInput(address); 
            return content.c_str(); 
        }
};
class menu_item_holding_register : public mibc {
public:
    menu_item_holding_register(uint16_t a) : mibc(a) {};
    virtual ~menu_item_holding_register(){};
    const char* GetValue() { 
        //if(current_device->update_holding_register)
            content = current_device->GetFormattedString_HoldingRegister(address); 
        return content.c_str(); 
    }
};
class menu_item_input_register : public mibc {
public:
    menu_item_input_register(uint16_t a) : mibc(a) {};
    virtual ~menu_item_input_register(){};
    const char* GetValue() {
        //if(current_device->update_input_registers)
            content = current_device->GetFormattedString_InputRegister(address); 
        return content.c_str(); 
    }
};

void buildDeviceDetail(uint16_t current_selection) {
    //find the device to display, exit if we do not find it
    current_device = std::find(_gDevice_List.begin(), _gDevice_List.end(), current_selection);
    if (current_device == _gDevice_List.end())
        return;

    cleanDeviceDetailsMenus();
    size_t size;
    uint16_t item_number;
    current_device->force_update();

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
    myname[item_number++] = { new menu_item_name, setname, "", item_number};
    //address
    myname[item_number++] = { new menu_item_address, setaddress, "", item_number};
    //the rest of it
    myname[item_number++] = { "Configure Device", ConfigureDevice, "", item_number};
    myname[item_number++] = { "Scan Device", scanDevice, "Find all registers offered by device", item_number };
    myname[item_number++] = { "Select Template", GetTemplateFilename, "", current_selection};
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
        myname[item_number++] = { new menu_item_coil(a), setcoil, "", item_number };
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
        myname[item_number++] = { new menu_item_discrete_input(a), DoNothing, "", item_number};
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
        myname[item_number++] = { new menu_item_holding_register(a), setHR, "", item_number};
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
        myname[item_number++] = { new menu_item_input_register(a), DoNothing, "", item_number};
    }
    #undef myname
     paintDeviceDetails();
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

bool GetTemplateFileName_callback(char** fieldbuf, size_t fieldsize) { 
    try {
        _gManager.LoadFile(*fieldbuf);
        current_device->template_name = *fieldbuf;
        buildDeviceDetail(current_device->GetAddress());
    }
    catch (std::invalid_argument& ex) {
        errormsg(ex.what());
        return true;
    }
    return false;
}
void GetTemplateFilename(uint16_t) {
    const char* fieldname[] = { "Template File Name", 0 };
    TextInputBox_Fields(fieldname, DEFAULT_FIELD_LENGTH, GetTemplateFileName_callback, 0);
}

bool ConfigureDevice_callback(char** fieldbuf, size_t fieldsize) {
    //ToDo finish this function.
    try {
        //error check input before configureing device
            uint32_t temp_object; //removes compiler warning
            for(char** p = fieldbuf; *p; p++)
                temp_object = std::stoul( *p, nullptr, 10);
        current_device->configureCoil(fieldbuf[0], fieldbuf[1]);
        current_device->configureDiscrete_input(fieldbuf[2], fieldbuf[3]);
        current_device->configureHolding_register(fieldbuf[4], fieldbuf[5]);
        current_device->configureInput_register(fieldbuf[6], fieldbuf[7]);
        buildDeviceDetail(current_device->GetAddress());
    }catch (std::invalid_argument& ex) {
        errormsg(ex.what());
        return true;
    }
    return false;
}
void ConfigureDevice(uint16_t value) {

    const char* fieldname[] = { "1st Coil Address:", "Coil Count:",
                                "1st DI Address:",   "DI Count:",
                                "1st HR Address:",   "HR Count:",
                                "1st IR Address:",   "IR Count:", 0 };
    #define ts( a ) std::to_string(current_device->a)
    std::string text_dump[] = { ts(GetStartCoil()),             ts(GetCoilN()), 
                                ts(GetStartDiscreteInput()),    ts(GetDiscreteInputN()), 
                                ts(GetStartHoldingRegister()),  ts(GetHoldingRegisterN()),
                                ts(GetStartInputRegister()),    ts(GetInputRegisterN())};
    #undef ts
    int i = 0;
    const char* fielddefault[] = { text_dump[i++].c_str(), text_dump[i++].c_str(),
                                   text_dump[i++].c_str(), text_dump[i++].c_str(),
                                   text_dump[i++].c_str(), text_dump[i++].c_str(),
                                   text_dump[i++].c_str(), text_dump[i++].c_str(), 0 };

    TextInputBox_Fields(fieldname, DEFAULT_FIELD_LENGTH, ConfigureDevice_callback, fielddefault);
}