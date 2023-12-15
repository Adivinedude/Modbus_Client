#include "menu_devices.h"
#include "../tui.h"
#include "../main.h"
#include <stdlib.h>
#include <sstream>
#include "../deviceDetailDisplay.h"

//find functions
#include <algorithm>

void AddDevice(const uint16_t value);
void RemoveDevice(const uint16_t value);

menu SubMenuDevices[] =
{
    { "Add",    AddDevice,          "Add Device to viewer"},
    { "Remove", RemoveDevice,       "Remove Device from viewer"},
    { "Scan",   NotYetImplemented,  "Scan for connected devices"},
    END_OF_MENU
};

menu_state SubMenuDevicesState = { SubMenuDevices, 0, 0, 1};

void Menu_Callback_Devices(const uint16_t value)
{
    if (_gDevice_List.size()) {
        SubMenuDevices[1].value = 0;
    }else{
        SubMenuDevices[1].value = MENU_DISABLE;
    }
    domenu(&SubMenuDevicesState);
}

enum name_of_fields { Device_Name = 0, Modbus_Address, spacer, Coil_Address, Coil_Count, DI_Address, DI_Count, HR_Address, HR_Count, IR_Address, IR_Count};

bool ProcessData_AddDevice(char** fieldbuf, size_t fieldsize) {
    //create the device on the list
    _gDevice_List.push_back(cModbus_Device());
    //get an iterator for the new device
    std::list<cModbus_Device>::iterator iter = --(_gDevice_List.end());
    try {

        iter->SetName                  (fieldbuf[Device_Name]);
        iter->SetAddress               (fieldbuf[Modbus_Address]);
        iter->configureCoil            (fieldbuf[Coil_Address],    fieldbuf[Coil_Count]);
        iter->configureDiscrete_input  (fieldbuf[DI_Address],      fieldbuf[DI_Count]);
        iter->configureHolding_register(fieldbuf[HR_Address],      fieldbuf[HR_Count]);
        iter->configureInput_register  (fieldbuf[IR_Address],      fieldbuf[IR_Count]);

        //see if the address or name has been taken.
        std::list<cModbus_Device>::iterator findIter;
        findIter = std::find(_gDevice_List.begin(), _gDevice_List.end(), *iter);

        //is if the found item is our new item.
        if (findIter == iter) {
            uint16_t addr = iter->GetAddress();
            _gDevice_List.sort(); 
            statusmsg("Device added");
            BuildDeviceListWindow();//recreate the menu object for displaying device
            buildDeviceDetail(addr);
            paintDeviceDetails();
            return false;
        }else{
            throw std::invalid_argument("A device with that name or address already exist");
        }
    }
    catch (std::invalid_argument const& ex) {
        _gDevice_List.erase(iter);
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
#ifdef _DEBUG
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
#else
    TextInputBox_Fields(fieldname, DEFAULT_FIELD_LENGTH, ProcessData_AddDevice, 0);
#endif
    
}

void RemoveDeviceSubmenu(const uint16_t value) {

    std::list<cModbus_Device>::iterator iter = std::find(_gDevice_List.begin(), _gDevice_List.end(), value);
    if (iter != _gDevice_List.end()) {
        _gDevice_List.erase(iter);
        statusmsg("Device Removed");
        BuildDeviceListWindow();
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
                *_pM = {buf.str(), RemoveDeviceSubmenu, 0, iter->GetAddress() };
            }
            domenu(&ms);
        }
        delete[] ms.pMenu;
    }
}