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
//and menu structors ..... in progress
#include "tui.h"                    // text user interface
#include "modbus.h"
#include "menu_main.h"              // Definition of the Root menu
#include "deviceDetailDisplay.h"    // Definition of the Main Display (just a bunch of menu objects)
#include "fstream_redirect.h"       // Recovers std::cerr and stderr streams into one stringstream object.
#include "cModbus_Template_Manager.h"
#include "Modbus_Poller.h"
#include <chrono>
#include <thread>

//Not very portable but easier than adding project settings 
//as visual studio is not as make file firendly as other toolchains
#pragma comment(lib, "..\\libmodbus\\src\\win32\\modbus.lib")   //Link the modbus.dll
#pragma comment(lib, "..\\PDCurses\\wincon\\pdcurses.lib")      //Link the curses library

// Global items
std::list<cModbus_Device>   _gDevice_List;          //list of known device
volatile bool               _gIsConnected = false;  //serial port status
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

void testfunction() {
    int a = 0;
    a++;
    a *= 2;
    return;
}

int main(int argc, char **argv)
{  
#ifdef _DEBUG
    
    //Got tired of typing this in every time.
    try{
        _gDevice_List.push_back(cModbus_Device());
        std::list<cModbus_Device>::iterator d = _gDevice_List.begin();
        d->template_name = "PCM_Modbus_Template.txt"; 
        d->SetName("test 1");
        d->SetAddress("1");
        d->configureCoil(0, 16);
        d->configureDiscrete_input(0, 8);
        d->configureHolding_register(0, 16);
        d->configureInput_register(0,23); 
        //d->update_coils = 1;
        //d->update_discrete_input = 1;
        //d->update_holding_register = 1;
        //d->update_input_registers = 1;
        _gManager.LoadFile("PCM_Modbus_Template.txt");
        _gManager.LoadFile("PCM_Modbus_Template.txt");   
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what();
        system("pause");
        return 0;
    }
    
#endif
    lock_device_list();

    //set callback functions
    _loop = &loop;
    _keyLoop = &keyLoop;

    //setip redraw body function
    SetupBodyWindow();

    //Set application run switch to on.
    restart = 1;

    //setup main menu
    setupmenu(&MainMenuState, "Modbus Client Demo - for PCM_Modbus");
     
    if (atexit(CleanupOnExit)) {
        beep();
        errormsg( "atexit() Failed" );
    }
    //setup body
    BuildDeviceList();
    buildDeviceDetail(0);

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
                exMenu(&sDetailList, 0, 0);
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
//    beep();
    stop_poller();
    unlock_device_list();
    if (sDetailList.pMenu){
        delete[] sDetailList.pMenu; // clear previous menu
        sDetailList.pMenu = 0;
    }
    cleanDeviceDetailsMenus();
    cleanDeviceDetailsWindow();
    if (sDetailList.pWindow){
        delwin(sDetailList.pWindow);
        sDetailList.pWindow = 0;
    }
    if (pDeviceDetailsWindow){
        delwin(pDeviceDetailsWindow);
        pDeviceDetailsWindow = 0;
    }
    cleanup();
}

//extern modbus_t* modbus;

time_t redraw_timer = 0;
void loop() {
    std::string rt;
    unlock_device_list();
    //modbus_poller();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //pipe stderr and std::cerr to the error readout
    _gError_redirect.update();
    if ( (rt = _gError_buffer.str()).size() ) {
        errormsg( rt.c_str() );
        _gError_buffer.str(std::string());
        _gError_buffer.clear();
    }
    lock_device_list();
    //redraw details if the device has updated
    if( redraw_timer+1 <= current_time){
        if(current_device != _gDevice_List.end())   //prevents exception when current_device is not valid
            repaintBody();
        redraw_timer = current_time;
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