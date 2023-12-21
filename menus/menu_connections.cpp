#include "menu_connections.h"
#include "../tui.h"
#include "modbus.h"
#include "../SerialHandler.h"
#include "../main.h"
#include <iostream>
#include "modbus.h"
#include "../Modbus_Poller.h"

SerialHandler _serial;

void ConnectRTU(const uint16_t value);
void ConnectTCP(const uint16_t value);
void Disconnect(const uint16_t value);

extern bool restart;

void QuitProgram(const uint16_t value) {
    stop_poller();
    if (modbus) {
        _gIsConnected = false;
        modbus_close(modbus);
        modbus_free(modbus);
        modbus = 0;
    }
    _loop = 0;
    DoExit(0);
    restart = 0;
}

void Menu_Callback_Connect(const uint16_t value)
{
    //enable/disable menu options
    menu SubMenuConnect[] =
    {
        { "Connect/RTU",ConnectRTU, "Establish a Modbus RTU Connection"},
        { "Connect/TCP",ConnectTCP, "Establish a Modbus TCP Connection"},
        { "Disconnect", Disconnect, "Close Connection" },
        { "Exit",       QuitProgram, "Terminate program" },
        END_OF_MENU
    };
    
    menu_state SubMenuConnectState = { SubMenuConnect, 0, 0, 1 };

    if (_gIsConnected) {
        SubMenuConnect[0].value = MENU_DISABLE;
        SubMenuConnect[1].value = MENU_DISABLE;
        SubMenuConnect[2].value = 0x00;
    }
    else {
        SubMenuConnect[0].value = 0x00;
        SubMenuConnect[1].value = 0x00;
        SubMenuConnect[2].value = MENU_DISABLE;
    }
    SubMenuConnectState.hotkey = true;
    domenu(&SubMenuConnectState);
}

void ConnectRTUSubmenu(const uint16_t value);
//build another sub menu populated with com ports
void ConnectRTU(const uint16_t value) {

    std::vector<std::string>* _list = _serial.GetAvailableComPorts();
    size_t s = _list->size();
    if (s) {
        //associate memory for new menu
        menu_state ms;
        ms.pMenu = new menu[s+1];
        ms.pMenu[s] = END_OF_MENU;
        {
            std::vector<std::string>::iterator iter = _list->begin();
            menu* _pM = ms.pMenu;
            char empty_cstr[] = "";
            for (uint16_t i = 0 ; iter != _list->end(); iter++, i++, _pM++) {
                *_pM = { new simple_menu_text(iter->c_str()), ConnectRTUSubmenu, "", i };
            }
            domenu( &ms);
        }
        delete[] ms.pMenu;
    }
}

void ConnectRTUSubmenu(const uint16_t value) {
    key = KEY_ESC;
    //easier and cleaner way to do this. ToDo properly use this vector
    std::vector<std::string>* _list = _serial.GetList();
    std::vector<std::string>::iterator iter = _list->begin();
    for (size_t a = value; a; a--) {
        if (iter != _list->end())
            iter++;
    }
    //if everything goes right, we will never reach _list->end()
    if (iter == _list->end())
        return;
    //sample string = "COM1: \device\usb0"
    std::size_t found = iter->find(":");
    if (found == std::string::npos)
        return;
    std::string temp;
    temp.append("\\\\.\\");
    temp.append(iter->substr(0, found));

    connect_poller( temp.c_str() );
}

void ConnectTCP(const uint16_t value) {
    NotYetImplemented(0);
}

void Disconnect(const uint16_t value) {
    if (_gIsConnected) {
        stop_poller();
        modbus_close(modbus);
        modbus_free(modbus);
        modbus = NULL;
        _gIsConnected = false;
        statusmsg("Disconnected");
    }
    key = KEY_ESC;
}