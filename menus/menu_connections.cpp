#include "menu_connections.h"
#include "../tui.h"
#include "modbus.h"
#include "../SerialHandler.h"
#include "../main.h"

SerialHandler _serial;

void ConnectRTU(const uint16_t value);
void ConnectTCP(const uint16_t value);
void Disconnect(const uint16_t value);

extern bool restart;
extern modbus_t* modbus;

void QuitProgram(const uint16_t value) {
    if (modbus) {
        modbus_close(modbus);
        modbus_free(modbus);
        modbus = 0;
    }
    DoExit(0);
    restart = 0;
}

menu SubMenuConnect[] =
{
    { "Connect/RTU",ConnectRTU, "Establish a Modbus RTU Connection"},
    { "Connect/TCP",ConnectTCP, "Establish a Modbus TCP Connection"},
    { "Disconnect", Disconnect, "Close Connection" },
    { "Exit",       QuitProgram, "Terminate program" },
    END_OF_MENU
};

menu_state SubMenuConnectState = { SubMenuConnect, 0, 0, 1};

void Menu_Callback_Connect(const uint16_t value)
{
    //enable/disable menu options
    
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
            for (char i = 0 ; iter != _list->end(); iter++, i++, _pM++) {
                _pM->name = iter->c_str();
                _pM->func = ConnectRTUSubmenu;
                _pM->desc = empty_cstr;
                _pM->value = i;
            }
            domenu( &ms);
        }
        delete[] ms.pMenu;
    }
}

void ConnectRTUSubmenu(const uint16_t value) {
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
    modbus = modbus_new_rtu(temp.c_str(), 9600, 'N', 8, 1);
    if (modbus == NULL) {
        errormsg("Connection Failed");
        return;
    }

    modbus_set_slave(modbus, 0);

    uint32_t old_response_to_sec;
    uint32_t old_response_to_usec;
    modbus_get_response_timeout(modbus, &old_response_to_sec, &old_response_to_usec);
    if (modbus_connect(modbus) == -1) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "Connection failed: %s", modbus_strerror(errno));
        errormsg(buf);
        modbus_free(modbus);
        return;
    }
    _gIsConnected = true;
    statusmsg("Connection Sucessful");
}

void ConnectTCP(const uint16_t value) {
    NotYetImplemented(0);
}

void Disconnect(const uint16_t value) {
    if (_gIsConnected) {
        modbus_close(modbus);
        modbus_free(modbus);
        modbus = NULL;
        _gIsConnected = false;
        statusmsg("Disconnected");
    }
}

/**************************** string entry box ****************************/

char* getfname(const char* desc, char* fname, int field)
{
    const char* fieldname[2];
    char* fieldbuf[1];

    fieldname[0] = desc;
    fieldname[1] = 0;
    fieldbuf[0] = fname;

    return (getstrings(fieldname, fieldbuf, field) == KEY_ESC) ? NULL : fname;
}
