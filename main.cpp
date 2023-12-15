/* This program is built on top of the following library and demo software
* Origion credits are as follows.
* 
 * Author : P.J. Kunst <kunst@prl.philips.nl>
 * Date   : 1993-02-25
 *
 * Purpose: This program demonstrates the use of the 'curses' library
 *          for the creation of (simple) menu-operated programs.
 *          In the PDCurses version, use is made of colors for the
 *          highlighting of subwindows (title bar, status bar etc).
 *
 * Acknowledgement: some ideas were borrowed from Mark Hessling's
 *                  version of the 'testcurs' program.
 */

#include "main.h"                   // globals shared across modules

//ToDo: completly rewrite this library to C++
//it is a pain in the ass to manage all the string dumps
//and menu structors
#include "tui.h"                    // text user interface
#include "modbus.h"
#include "menu_main.h"              // Definition of the Root menu
#include "deviceDetailDisplay.h"    // Definition of the Main Display (just a bunch of menu objects)
#include "fstream_redirect.h"       // Recovers std::cerr and stderr streams into one stringstream object.
#include "cModbus_Template_Manager.h"

//Not very portable but easier than adding project settings 
//as visual studio is not as make file firendly as other toolchains
#pragma comment(lib, "..\\libmodbus\\src\\win32\\modbus.lib")   //Link the modbus.dll
#pragma comment(lib, "..\\PDCurses\\wincon\\pdcurses.lib")      //Link the curses library

// Global items
std::list<cModbus_Device>  _gDevice_List;          //list of known device
menu_state                  _gDeviceListMenuState;  //menu object for device list
bool                        _gIsConnected = false;  //serial port status
std::stringstream           _gError_buffer;         //storage for error messages
fstream_redirect            _gError_redirect(_gError_buffer,    //Capture stderr and std::cerr messages
                                                std::cerr, 
                                                stderr);

void loop();         // Run code during idle time
int  keyLoop(int);   // Pre-processing of key strokes
void CleanupOnExit();// Function to execute at program termination

/***************************** start main menu  ***************************/
bool restart = 0;
//WINDOW *pDeviceListWindow = 0, *pDeviceDetailsWindow = 0;

int main(int argc, char **argv)
{   
#ifdef _DEBUG
    try{
        _gDevice_List.push_back(cModbus_Device());
        std::list<cModbus_Device>::iterator d = _gDevice_List.begin();
        d->template_name = "..\\PCM_Modbus_Template.txt"; 
        d->SetName("test 1");
        d->SetAddress("1");
        d->configureCoil(0, 16);
        d->configureDiscrete_input(0, 8);
        d->configureHolding_register(0, 16);
        d->configureInput_register(0,22);
        
        _gManager.LoadFile("..\\PCM_Modbus_Template.txt");
    }
    catch (const std::exception& ex) {
        std::cout << ex.what();
        return 0;
    }
#endif

    //set callback functions
    _loop = &loop;
    _keyLoop = &keyLoop;

    //Set application run switch to on.
    restart = 1;

    //setup main menu
    setupmenu(&MainMenuState, "Modbus Client Demo - for PCM_Modbus");
     
    if (atexit(CleanupOnExit)) {
        beep();
        errormsg( "atexit() Failed" );
    }

    //setup body
    BuildDeviceListWindow();

    int selection = 0;
    //main loop
    while (restart)
    {
        //handle selection and key presses of main window items.
        //  Main Menu
        //  Device List
        //  Device Details
        switch (selection) {
            //handle main menu
            //when the user exits the main menu (presses down) jump to device list
            case 0:
                mainmenu(&MainMenuState);
                selection = 1;
                break;

            //handle Device List
            //when the user left exit to main menu, 
            //when the user presses right, exit to device details
            case 1:
                exMenu(&_gDeviceListMenuState, 0, 0);
                switch (key) {
                    case '\n':
                    case KEY_RIGHT:
                        selection = 2;
                        break;

                    case KEY_ESC:
                    case KEY_LEFT:
                        selection = 0;
                        break;
                }
                key = ERR;
                break;
            //handle device details
            case 2:
                if (!_gDevice_List.size()) {
                    selection = 0;
                    key = ERR;
                    break;
                }
                handleDeviceDetail();
                switch (key) {
                    case KEY_ESC:
                    case KEY_LEFT:
                        selection--;
                        break;
                }
                key = ERR;
                break;
            default:
                selection = 0;            
        }
    } 
    return 0;
}

void CleanupOnExit(){
    beep();
    if (_gDeviceListMenuState.pMenu)
        delete[] _gDeviceListMenuState.pMenu; // clear previous menu
    cleanDeviceDetailsMenus();
    cleanDeviceDetailsWindow();
    if (_gDeviceListMenuState.pWindow)
        delwin(_gDeviceListMenuState.pWindow);
    if (pDeviceDetailsWindow)
        delwin(pDeviceDetailsWindow);
    cleanup();
}

//extern modbus_t* modbus;

void loop() {
    //Read from the error buffer and write to the error window
 //   if(modbus)
 //       modbus->
    _gError_redirect.update();
    if (_gError_buffer.gcount()) {
        errormsg( _gError_buffer.str().c_str() );
        _gError_buffer.str(std::string());
        _gError_buffer.clear();
    }
}
int keyLoop(int c) {
    /*
    std::string buf;
    buf.append("Key Pressed: ");
    buf.append(std::to_string(c));
    statusmsg(buf.c_str());
    */
    switch (c) {
        //use keypad enter button
        case PADENTER:
            c = '\n';
            break;
        case KEY_ESC:
            rmerror();
            break;
    }
    return c;
}

void OnSelectDevice(const uint16_t value) {  
    buildDeviceDetail(value); 
    paintDeviceDetails(); 
}
void OnClickDevice(const uint16_t value) {

}
void BuildDeviceListWindow() {
    size_t s;

    // clear previous menu
    if( _gDeviceListMenuState.pMenu )
        delete[] _gDeviceListMenuState.pMenu;
    // clear the previous window
    if (_gDeviceListMenuState.pWindow)
        delwin(_gDeviceListMenuState.pWindow);

    //make new meue for the device list
    if (s = _gDevice_List.size() ) {
        _gDeviceListMenuState.pMenu = new menu[s + 2];
    }else {
        _gDeviceListMenuState.pMenu = new menu[2];
    }
    //add menu header
    _gDeviceListMenuState.pMenu[0] = { "Device List", DoNothing, "", MENU_DISABLE };

    //add each device to the menu
    {
        menu* p = _gDeviceListMenuState.pMenu;
        p++;
        for (std::list<cModbus_Device>::iterator iter = _gDevice_List.begin(); 
                iter != _gDevice_List.end(); iter++, p++) {
            *p = { iter->GetName(), OnClickDevice, "", iter->GetAddress(), OnSelectDevice};
        }
    }
    //terminate the menu
    _gDeviceListMenuState.pMenu[s+1] = END_OF_MENU;

    int y, x;
    //Get the menu dimensions
    menudim(_gDeviceListMenuState.pMenu, &y, &x);
    //make new window for the menu
    _gDeviceListMenuState.pWindow = derwin(bodywin(), y+2, x+2, 0, 0);
    clsbody();                                      //clear main body window
    colorbox(_gDeviceListMenuState.pWindow, SUBMENUCOLOR, 1);   //set color and outline
    repaintmenu(&_gDeviceListMenuState);  //paint new menu
    touchwin(bodywin());                            //paint body window
    wrefresh(bodywin());
}