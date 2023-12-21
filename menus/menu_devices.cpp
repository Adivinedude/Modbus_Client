#include "menu_devices.h"
#include "../tui.h"
#include "../main.h"
#include <stdlib.h>
#include <sstream>
#include "../deviceDetailDisplay.h"
#include "../Modbus_Poller.h"

//find functions
#include <algorithm>

void AddDevice(const uint16_t value);
void RemoveDevice(const uint16_t value);


void Menu_Callback_Devices(const uint16_t value)
{
    menu SubMenuDevices[] =
    {
        { "Add",    AddDevice,          "Add Device to viewer"},
        { "Remove", RemoveDevice,       "Remove Device from viewer"},
        { "Scan",   scan_network_for_devices,  "Scan for connected devices"},
        END_OF_MENU
    };

    menu_state SubMenuDevicesState = { SubMenuDevices, 0, 0, 1 };

    if (_gDevice_List.size()) {
        SubMenuDevices[1].value = 0;
    }else{
        SubMenuDevices[1].value = MENU_DISABLE;
    }
    SubMenuDevicesState.hotkey = true;
    domenu(&SubMenuDevicesState);
}

enum name_of_fields { Device_Name = 0, Modbus_Address, spacer, Coil_Address, Coil_Count, DI_Address, DI_Count, HR_Address, HR_Count, IR_Address, IR_Count};

bool ProcessData_AddDevice(char** fieldbuf, size_t fieldsize) {
    cModbus_Device newdevice;
    try {

        newdevice.SetName                  (fieldbuf[Device_Name]);
        newdevice.SetAddress               (fieldbuf[Modbus_Address]);
        //see if the address or name has been taken.
        auto findIter = std::find(_gDevice_List.begin(), _gDevice_List.end(), newdevice);
        if( findIter == _gDevice_List.end() ){
            newdevice.configureCoil            (fieldbuf[Coil_Address],    fieldbuf[Coil_Count]);
            newdevice.configureDiscrete_input  (fieldbuf[DI_Address],      fieldbuf[DI_Count]);
            newdevice.configureHolding_register(fieldbuf[HR_Address],      fieldbuf[HR_Count]);
            newdevice.configureInput_register  (fieldbuf[IR_Address],      fieldbuf[IR_Count]);

            uint16_t addr = newdevice.GetAddress();
            _gDevice_List.push_back(newdevice);
            _gDevice_List.sort(); 
            statusmsg("Device added");
            BuildDeviceList();//recreate the menu object for displaying device
            buildDeviceDetail(addr);
            return false;
        }else{
            throw std::invalid_argument("A device with that name or address already exist");
        }
    }
    catch (std::invalid_argument const& ex) {
        errormsg(ex.what());
    }
    return true;
}

void AddDevice(const uint16_t value)
{
    const char* fieldname[] =    { "Device Name:", "ModBus Address:", 
                                   "",
                                   "1st Coil Address:", "Coil Count:",
                                   "1st DI Address:",   "DI Count:", 
                                   "1st HR Address:",   "HR Count:", 
                                   "1st IR Address:",   "IR Count:", 0};
//#ifdef _DEBUG
    /*
    std::string temp, temp_two;
    temp.append("Test ");
    temp.append(std::to_string(_gDevice_List.size()+1));
    temp_two.append(std::to_string((_gDevice_List.size() + 1) * _gDevice_List.size()+1));
    const char* fielddefault[] = { temp.c_str(), temp.c_str()+5,
                                   "",
                                   temp_two.c_str(), temp_two.c_str(),
                                   temp_two.c_str(), temp_two.c_str(),
                                   temp_two.c_str(), temp_two.c_str(),
                                   temp_two.c_str(), temp_two.c_str(), 0 };
    TextInputBox_Fields(fieldname, DEFAULT_FIELD_LENGTH, ProcessData_AddDevice, fielddefault);
    */
//#else
    TextInputBox_Fields(fieldname, DEFAULT_FIELD_LENGTH, ProcessData_AddDevice, 0);
//#endif
    
}

void RemoveDeviceSubmenu(const uint16_t value) {

    std::list<cModbus_Device>::iterator iter = std::find(_gDevice_List.begin(), _gDevice_List.end(), value);
    if (iter != _gDevice_List.end()) {
        stop_poller();
        if (iter == current_device)
            cleanDeviceDetailsMenus();
        current_device = _gDevice_List.end();
        _gDevice_List.erase(iter);
        statusmsg("Device Removed");
        BuildDeviceList();
        start_poller();
        key = KEY_ESC;
    }

}

//build another sub menu populated with com ports
void RemoveDevice(const uint16_t value) {
    
    size_t s = _gDevice_List.size();
    if (s) {
        menu_state ms;
        //associate memory for new menu
        ms.pMenu = new menu[s + 1];
        ms.pMenu[s] = END_OF_MENU;
        {
            std::list<cModbus_Device>::iterator iter = _gDevice_List.begin();
            menu* _pM = ms.pMenu;

            for (unsigned char i = 0; iter != _gDevice_List.end(); iter++, i++, _pM++) {
                std::stringstream buf;
                buf << std::dec << iter->GetAddress() << ": " << iter->GetName();
                *_pM = { new simple_menu_text(buf.str()), RemoveDeviceSubmenu, std::string(), iter->GetAddress()};
            }
            domenu(&ms);
        }
        delete[] ms.pMenu;
    }
}