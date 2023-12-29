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
void ConnectionOptions(const uint16_t value);

extern volatile bool restart;

void QuitProgram(const uint16_t value) {
    restart = 0;
    key = KEY_ESC;
    DoExit(0);
}
void Menu_Callback_Connect(const uint16_t value)
{
    //enable/disable menu options
    menu SubMenuConnect[] =
    {
        { "Connect/RTU",    ConnectRTU,         "Establish a Modbus RTU Connection"},
        { "RTU Options",    ConnectionOptions,  "Configure RTU" },
        { "Connect/TCP",    ConnectTCP,         "Establish a Modbus TCP Connection"},
        { "Disconnect",     Disconnect,         "Close Connection" },
        { "Exit",           QuitProgram,        "Terminate program" },
        END_OF_MENU
    };
    
    menu_state SubMenuConnectState = { SubMenuConnect, 0, 0, 1 };

    if (_gIsConnected) {
        SubMenuConnect[0].value = MENU_DISABLE;
        SubMenuConnect[1].value = MENU_DISABLE;
        SubMenuConnect[2].value = MENU_DISABLE;
        SubMenuConnect[3].value = 0x00;
    }
    else {
        SubMenuConnect[0].value = 0x00;
        SubMenuConnect[1].value = 0x00;
        SubMenuConnect[2].value = 0x00;
        SubMenuConnect[3].value = MENU_DISABLE;
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

    connect_modbus( temp.c_str() );
}
void ConnectTCP(const uint16_t value) {
    NotYetImplemented(0);
}
void Disconnect(const uint16_t value) {
    disconnect_modbus();
    statusmsg("Disconnected");
    key = KEY_ESC;
}
bool ProcessData_ConnectionOptions(char** fieldbuf, size_t fieldsize) {
    sConnection_Options o;
    try {
        o.baudrate  = std::stoi( fieldbuf[baud], nullptr, 10);
        o.parity    = fieldbuf[parity][0];
        o.data_bit  = std::stoi( fieldbuf[data_bit], nullptr, 10);
        o.stop_bit  = std::stoi( fieldbuf[stop_bit], nullptr, 10);

        //error check the user input
        switch (o.baudrate) {
            case 110:
            case 300:
            case 600:
            case 1200:
            case 2400:
            case 4800:
            case 9600:
            case 14400:
            case 19200:
            case 38400:
            case 57600:
            case 115200:
            case 128000:
            case 256000:
                break;
            default:
                std::cerr << "Invalid Baudrate" << std::endl;
                return true;
        }
        switch (o.parity) {
            case 'E':
            case 'e':
            case 'N':
            case 'n':
            case 'O':
            case 'o':
                break;
            default:
                std::cerr << "Invalid parity" << std::endl;
                return true;
        }
        if (o.data_bit < 5 || o.data_bit > 8) {
                std::cerr << "Invalid data bit" << std::endl;
                return true;
        }
        if (o.stop_bit < 1 || o.stop_bit > 2) {
            std::cerr << "Invalid stop bit" << std::endl;
            return true;
        }
        Current_Connection_Settings.baudrate    = o.baudrate;
        Current_Connection_Settings.parity      = o.parity;
        Current_Connection_Settings.data_bit    = o.data_bit;
        Current_Connection_Settings.stop_bit    = o.stop_bit;

        return false;
    }
    catch (std::invalid_argument const& ex) {
        errormsg(ex.what());
    }
    return true;
}
void ConnectionOptions(const uint16_t value)
{
    const char* fieldname[] = { "Baudrate:", "Parity:", "Data bits:", "Stop bits", 0 };
    std::string b, p, d, s;
    b = std::to_string(Current_Connection_Settings.baudrate);
    p = Current_Connection_Settings.parity;
    d = std::to_string(Current_Connection_Settings.data_bit);
    s = std::to_string(Current_Connection_Settings.stop_bit);
    const char* defaultfields[] = { b.c_str(), p.c_str(), d.c_str(), s.c_str(), 0};

    TextInputBox_Fields(fieldname, DEFAULT_FIELD_LENGTH, ProcessData_ConnectionOptions, defaultfields);
}