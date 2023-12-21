#include "Modbus_Poller.h"
#include "main.h"
#include "deviceDetailDisplay.h"
#include "tui.h"
#include "modbus.h"
#include <iostream>
#include <Windows.h>
#include <thread>
#include <mutex>
#include "./menus/menu_connections.h"   //disconnect function
#include "./menus/menu_devices.h"       //add device function

#define POLL_TIME_INTERVAL 1000
#define MAXIUM_DEVICE_ADDRESS 247
#define MAXIUM_REGISTER_ADDRESS 65536

modbus_t* volatile modbus = 0;
volatile bool poll_modbus_thread_run;

std::thread myThread;
std::mutex  device_list_lock;
std::mutex  modbus_address_lock;
volatile uint8_t poller_state = 0;

void scan_network();
void modbus_poller();
void scan_device_for_registers();

void connect_poller(const char* name) {

    modbus = modbus_new_rtu(name, 9600, 'N', 8, 1);
    //modbus_set_debug(modbus, false);
    uint32_t sec, usec;
    int status = modbus_get_response_timeout(modbus, &sec, &usec);
    status = modbus_set_response_timeout(modbus, 0, 500000);
    try{
        //if (modbus_set_error_recovery(modbus, (modbus_error_recovery_mode)(MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL)) == -1)
        //    throw std::exception("Set error recovery failed");
        if (modbus_connect(modbus) == -1) 
            throw std::exception("Connection Failed");
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << ": " << modbus_strerror(errno) << std::endl;
        modbus_free(modbus);
        modbus = 0;
        _gIsConnected = false;
        return;
    }

    _gIsConnected = true;
    statusmsg("Connection Sucessful");
    start_poller();
}
void poll_modbus_thread() {
    poll_modbus_thread_run = true;
    while (poll_modbus_thread_run) {
        switch (poller_state) {
            case 0:
                modbus_poller();
                Sleep(POLL_TIME_INTERVAL);
                break;
            case 1:
                scan_network();
                poller_state = 0;
                break;
            case 2:
                scan_device_for_registers();
                poller_state = 0;
                break;
        }
    }
}
void start_poller() {
    myThread = std::thread(poll_modbus_thread);
}
void stop_poller() {
    if( poll_modbus_thread_run ){
        poll_modbus_thread_run = false;
        device_list_lock.unlock();
        myThread.join();
        device_list_lock.lock();
    }
}
void lock_poller() {
    device_list_lock.lock();
}
void unlock_poller() {
    device_list_lock.unlock();
}
void modbus_poller() {
    if( !_gIsConnected )
        return;
    int         status_bit = -1,
                status_reg = -1;
    DWORD       error;
    size_t      num_of_registers;
    uint16_t    c_address;
    uint8_t     *buffer8 = 0;
    uint16_t    *buffer16 = 0;
    char buf[256];

    for (std::list<cModbus_Device>::iterator it = _gDevice_List.begin(); it != _gDevice_List.end(); it++) {
        //is it time to update
        if (it->last_update_time + 1 < current_time) {
            it->last_update_time = current_time;
            modbus_t* mb = modbus;
            modbus_address_lock.lock();
            if (modbus_set_slave(modbus, c_address = it->GetAddress()) == -1) {
                std::cerr <<  "Invalid slave ID" << std::endl;
                modbus_free(modbus);
                modbus = 0;
                _gIsConnected = false;
                return;
            }

            //write coils
            num_of_registers    = it->GetCoilN();
            if(num_of_registers){
                c_address           = it->GetStartCoil();
                buffer8             = it->GetBufferCoil();
                status_bit = modbus_write_bits( modbus, c_address, num_of_registers, buffer8);
                if(status_bit == -1){
                    std::cerr << "Failed to write coils<" << std::dec << it->GetAddress() 
                                << ":" << std::hex << c_address << "-" << std::dec << num_of_registers
                                << "> " << modbus_strerror(errno);
                    error = GetLastError();
                    switch (error) {
                        case 0:
                            break;
                        default:
                            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                buf, (sizeof(buf) / sizeof(char)), NULL);
                    }

                }
                if(status_bit != num_of_registers)
                    std::cerr << "Not all coils written" << std::endl;
            }
            //read descrete inputs
            buffer8 = 0;
            num_of_registers = it->GetDiscreteInputN();
            if (num_of_registers) {
                c_address        = it->GetStartDiscreteInput();
                buffer8          = new uint8_t[num_of_registers?num_of_registers:1];
                status_bit = modbus_read_input_bits(modbus, c_address, num_of_registers, buffer8);
                if(status_bit != num_of_registers)
                    std::cerr << "Failed to read descrete inputs<" << std::dec << it->GetAddress()
                                << ":" << std::hex << c_address << "-" << std::dec << status_bit
                                << "> " << modbus_strerror(errno);
            }
            //write hold registers
            num_of_registers    = it->GetHoldingRegisterN();
            if (num_of_registers){
                c_address           = it->GetStartHoldingRegister();
                buffer16            = it->GetBufferHoldingRegister();
                status_reg = modbus_write_registers(modbus, c_address, num_of_registers, buffer16);
                if (status_reg == -1)
                    std::cerr << "Failed to write hold registers<" << std::dec << it->GetAddress()
                                << ":" << std::hex << c_address << "-" << std::dec << status_reg
                                << "> " << modbus_strerror(errno);
            }
            //read input registers
            buffer16 = 0;
            num_of_registers    = it->GetInputRegisterN();
            if (num_of_registers){
                c_address           = it->GetStartInputRegister();
                buffer16            = new uint16_t[num_of_registers?num_of_registers:1];
                status_reg = modbus_read_input_registers(modbus, c_address, num_of_registers, buffer16);
                if (status_reg != num_of_registers)
                    std::cerr << "Failed to read input registers<" << std::dec << it->GetAddress()
                                << ":" << std::hex << c_address << "-" << std::dec << status_reg
                                << "> " << modbus_strerror(errno);
            }
            modbus_address_lock.unlock();
            // store the data that was retrived
            device_list_lock.lock();
            if (status_bit != -1 && buffer8) {
                c_address = it->GetStartDiscreteInput();
                for (uint8_t* p = buffer8; status_bit; status_bit--, c_address++, p++) {
                    try {
                        it->SetDiscreteInput(c_address, *p);
                    }
                    catch (const std::invalid_argument& ex) {
                        std::cerr << ex.what() << std::endl;
                    }
                }
            }
            if (buffer8)
                delete[] buffer8;
            ////////////////
            if (status_reg != -1 && buffer16) {
                c_address = it->GetStartInputRegister();
                for (uint16_t* p = buffer16; status_reg; status_reg--, c_address++, p++) {
                    try {
                        it->SetInputRegister(c_address, *p);
                    }
                    catch (const std::invalid_argument& ex) {
                        std::cerr << ex.what() << std::endl;
                    }
                }
            }
            if (buffer16)
                delete[] buffer16;
            device_list_lock.unlock();
        }
    }
}
void scan_device_for_registers(const uint16_t) {
    poller_state = 2;
    key = KEY_ESC;
}
void scan_device_for_registers() {
 
    uint32_t    coil_start, coil_stop,
                di_start,   di_stop,
                hr_start,   hr_stop,
                ir_start,   ir_stop;
    int status;
    uint8_t bit;
    uint16_t reg;
    if (!modbus) {
        std::cerr << "Not connected. Can not scan" << std::endl;
        return;
    }
    if (current_device == _gDevice_List.end())
        return;

    modbus_address_lock.lock();
    modbus_flush(modbus);
    if (modbus_set_slave(modbus, current_device->GetAddress()) == -1) {
        std::cerr << "Invalid slave ID" << std::endl;
        modbus_free(modbus);
        _gIsConnected = false;
        return;
    }
    //coils
    for (coil_start = 0; coil_start <= MAXIUM_REGISTER_ADDRESS; coil_start++) {
        status = modbus_read_bits(modbus, coil_start, 1, &bit);
        if (status != -1)
            break;
    }
    for (coil_stop = coil_start + 1; coil_stop <= MAXIUM_REGISTER_ADDRESS; coil_stop++) {
        status = modbus_read_bits(modbus, coil_stop, 1, &bit);
        if(status == -1)
            break;
    }
    //di
    for (di_start = 0; di_start <= MAXIUM_REGISTER_ADDRESS; di_start++) {
        status = modbus_read_input_bits(modbus, di_start, 1, &bit);
        if (status != -1)
            break;
    }
    for (di_stop = di_start + 1; di_stop <= MAXIUM_REGISTER_ADDRESS; di_stop++) {
        status = modbus_read_input_bits(modbus, di_stop, 1, &bit);
        if (status == -1)
            break;
    }
    //hold registers
    for (hr_start = 0; hr_start <= MAXIUM_REGISTER_ADDRESS; hr_start++) {
        status = modbus_read_registers(modbus, hr_start, 1, &reg);
        if (status != -1)
            break;
    }
    for (hr_stop = hr_start + 1; hr_stop <= MAXIUM_REGISTER_ADDRESS; hr_stop++) {
        status = modbus_read_registers(modbus, hr_stop, 1, &reg);
        if (status == -1)
            break;
    }
    //input registers
    for (ir_start = 0; ir_start <= MAXIUM_REGISTER_ADDRESS; ir_start++) {
        status = modbus_read_input_registers(modbus, ir_start, 1, &reg);
        if (status != -1)
            break;
    }
    for (ir_stop = ir_start + 1; ir_stop <= MAXIUM_REGISTER_ADDRESS; ir_stop++) {
        status = modbus_read_input_registers(modbus, ir_stop, 1, &reg);
        if (status == -1)
            break;
    }
    modbus_address_lock.unlock();
    device_list_lock.lock();
    current_device->configureCoil(              coil_start, coil_stop   - coil_start);
    current_device->configureDiscrete_input(    di_start,   di_stop     - di_start);
    current_device->configureHolding_register(  hr_start,   hr_stop     - hr_start);
    current_device->configureInput_register(    ir_start,   ir_stop     - ir_start);
    buildDeviceDetail(current_device->GetAddress());
    device_list_lock.unlock();
}
void scan_network_for_devices(const uint16_t)
{
    poller_state = 1;
    key = KEY_ESC;
}
void scan_network(){
    int status;
    uint8_t bit;
    if (!modbus) {
        std::cerr << "Not connected. Can not scan" << std::endl;
        return;
    }
    status = modbus_set_response_timeout(modbus, 0, 100000);
    for (int address = 0; address <= MAXIUM_DEVICE_ADDRESS && poller_state == 2; address++) {
        modbus_address_lock.lock();
        std::cerr << "Pinging <" << address << ">" <<std::endl;
        if (modbus_set_slave(modbus, address) == -1) {
            std::cerr << "Invalid slave ID" << std::endl;
            modbus_free(modbus);
            _gIsConnected = false;
            modbus = 0;
            return;
        }
        status = modbus_read_bits(modbus, 1, 1, &bit);
        modbus_address_lock.unlock();
        switch(status){
            case -1:
                if (!errno)
                    break;  //check errno for timeout, device not online
                //device returned an error code, device is online. log it.

            case 1:
                //device online we recived the bits
                cModbus_Device a;
                a.SetAddress(address);
                a.SetName( std::to_string(address).c_str() );
                auto found = std::find(_gDevice_List.begin(), _gDevice_List.end(), a);
                if(found == _gDevice_List.end() ){
                    _gDevice_List.push_back(a);
                    BuildDeviceList();
                }
            break;
        }
        status = errno;
    }
}
