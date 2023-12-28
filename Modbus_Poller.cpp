#include "Modbus_Poller.h"
#include "main.h"
#include "deviceDetailDisplay.h"
#include "tui.h"
#include "modbus.h"
#include <iostream>
#include <Windows.h>
#include <thread>
#include <mutex>
#include <chrono>
#include "./menus/menu_connections.h"   //disconnect function
#include "./menus/menu_devices.h"       //add device function

#define POLL_TIME_INTERVAL 1000
#define MAXIUM_DEVICE_ADDRESS 247
#define MAXIUM_REGISTER_ADDRESS 65536

modbus_t* volatile modbus = 0;
volatile bool thread_state_Poller = false;
volatile bool thread_state_ScanNetwork = false;
volatile bool thread_state_ScanDevice = false;

volatile sConnection_Options Current_Connection_Settings = { 9600, 'N', 8, 1 };

std::thread tPoller, tScanNetwork, tScanDevice;
std::mutex  device_list_lock;
std::mutex  modbus_lock;

void scan_network();
void modbus_poller();
void ScanDevice();
void ScanNetwork();

void connect_modbus(const char* name) {

    modbus = modbus_new_rtu(name,
                            Current_Connection_Settings.baudrate,
                            Current_Connection_Settings.parity,
                            Current_Connection_Settings.data_bit,
                            Current_Connection_Settings.stop_bit);
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
void disconnect_modbus()
{
    stop_poller();
    stop_ScanDevice();
    stop_ScanNetwork();
    if (modbus) {
        modbus_close(modbus);
        modbus_free(modbus);
        modbus = 0;
    }
    _gIsConnected = false;
}
void start_poller() {
    if( !thread_state_Poller ){
        if (tPoller.joinable())
            tPoller.join();
        tPoller = std::thread(modbus_poller);
    }
}
void stop_poller() {
    if(thread_state_Poller){
        thread_state_Poller = false;
        device_list_lock.unlock();
        tPoller.join();
        device_list_lock.lock();
    }else{
        if (tPoller.joinable())
            tPoller.join();
    }
}
void start_ScanDevice() {
    if(!thread_state_ScanDevice){
        if( tScanDevice.joinable() )
            tScanDevice.join();
        tScanDevice = std::thread(ScanDevice);
    }
}
void stop_ScanDevice() {
    if (thread_state_ScanDevice) {
        thread_state_ScanDevice = false;
        device_list_lock.unlock();
        tScanDevice.join();
        device_list_lock.lock();
    }else{
        if( tScanDevice.joinable() )
            tScanDevice.join();
    }
}
void start_ScanNetwork() {
    if(!thread_state_ScanNetwork){
        if(tScanNetwork.joinable())
            tScanNetwork.join();
        tScanNetwork = std::thread(ScanNetwork);
    }
}
void stop_ScanNetwork() {
    if (thread_state_ScanNetwork) {
        thread_state_ScanNetwork = false;
        device_list_lock.unlock();
        tScanNetwork.join();
        device_list_lock.lock();
    }else{
        if( tScanNetwork.joinable())
            tScanNetwork.join();  
    }
}

void lock_device_list() {
    device_list_lock.lock();
}
void unlock_device_list() {
    device_list_lock.unlock();
}
void modbus_poller() {
if(thread_state_Poller)
    return;
thread_state_Poller = true;
    while(thread_state_Poller){
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_TIME_INTERVAL));
        if( !_gIsConnected ){
            continue;
        }
        int         status_bit = -1,
                    status_reg = -1;
        DWORD       error;
        size_t      num_of_registers;
        uint16_t    c_address;
        uint8_t     *buffer8 = 0;
        uint16_t    *buffer16 = 0;
        char buf[256];

        for (std::list<cModbus_Device>::iterator it = _gDevice_List.begin(); (it != _gDevice_List.end()) && thread_state_Poller; it++) {
            //is it time to update
            if (it->last_update_time + 1 < current_time) {
                it->last_update_time = current_time;
                modbus_t* mb = modbus;
                modbus_lock.lock();
                modbus_flush(modbus);
                if (modbus_set_slave(modbus, c_address = it->GetAddress()) == -1) {
                    std::cerr <<  "Invalid slave ID" << std::endl;
                    _gIsConnected = false;
                    modbus_lock.unlock();
                    continue;
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
                            //case 5: usb device has been disconnected from the pc. handle it!
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
                modbus_lock.unlock();
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
    thread_state_Poller = false;
}

void ScanDevice() {
    if(thread_state_ScanDevice)
        return;
    thread_state_ScanDevice = true;

    uint32_t    coil_start, coil_stop,
                di_start,   di_stop,
                hr_start,   hr_stop,
                ir_start,   ir_stop;
    int status;
    uint8_t bit;
    uint16_t reg;
    try{
        if( !_gIsConnected )
            throw std::invalid_argument("Not connected. Can not scan");
        if (!modbus)
            throw std::invalid_argument("invalid modbus pointer");
        if (current_device == _gDevice_List.end())
            throw std::invalid_argument("Device not found");
    }
    catch (std::invalid_argument& ex) {
        std::cerr << ex.what() << std::endl;
        thread_state_ScanDevice = false;
        return;
    }
    modbus_lock.lock();
    modbus_flush(modbus);
    if (modbus_set_slave(modbus, current_device->GetAddress()) == -1) {
        std::cerr << "Invalid slave ID" << std::endl;
        thread_state_ScanDevice = false;
        modbus_lock.unlock();
        return;
    }

    //coils
    for (coil_start = 0; (coil_start <= MAXIUM_REGISTER_ADDRESS) && thread_state_ScanDevice; coil_start++) {
        status = modbus_read_bits(modbus, coil_start, 1, &bit);
        if (status != -1)
            break;
    }
    for (coil_stop = coil_start + 1; (coil_stop <= MAXIUM_REGISTER_ADDRESS) && thread_state_ScanDevice; coil_stop++) {
        status = modbus_read_bits(modbus, coil_stop, 1, &bit);
        if(status == -1)
            break;
    }
    //di
    for (di_start = 0; (di_start <= MAXIUM_REGISTER_ADDRESS) && thread_state_ScanDevice; di_start++) {
        status = modbus_read_input_bits(modbus, di_start, 1, &bit);
        if (status != -1)
            break;
    }
    for (di_stop = di_start + 1; (di_stop <= MAXIUM_REGISTER_ADDRESS) && thread_state_ScanDevice; di_stop++) {
        status = modbus_read_input_bits(modbus, di_stop, 1, &bit);
        if (status == -1)
            break;
    }
    //hold registers
    for (hr_start = 0; (hr_start <= MAXIUM_REGISTER_ADDRESS) && thread_state_ScanDevice; hr_start++) {
        status = modbus_read_registers(modbus, hr_start, 1, &reg);
        if (status != -1)
            break;
    }
    for (hr_stop = hr_start + 1; (hr_stop <= MAXIUM_REGISTER_ADDRESS) && thread_state_ScanDevice; hr_stop++) {
        status = modbus_read_registers(modbus, hr_stop, 1, &reg);
        if (status == -1)
            break;
    }
    //input registers
    for (ir_start = 0; (ir_start <= MAXIUM_REGISTER_ADDRESS) && thread_state_ScanDevice; ir_start++) {
        status = modbus_read_input_registers(modbus, ir_start, 1, &reg);
        if (status != -1)
            break;
    }
    for (ir_stop = ir_start + 1; (ir_stop <= MAXIUM_REGISTER_ADDRESS) && thread_state_ScanDevice; ir_stop++) {
        status = modbus_read_input_registers(modbus, ir_stop, 1, &reg);
        if (status == -1)
            break;
    }
    modbus_lock.unlock();
    device_list_lock.lock();
    current_device->configureCoil(              coil_start, coil_stop   - coil_start);
    current_device->configureDiscrete_input(    di_start,   di_stop     - di_start);
    current_device->configureHolding_register(  hr_start,   hr_stop     - hr_start);
    current_device->configureInput_register(    ir_start,   ir_stop     - ir_start);
    buildDeviceDetail(current_device->GetAddress());
    device_list_lock.unlock();
    thread_state_ScanDevice = false;
}
void ScanNetwork(){
    if(thread_state_ScanNetwork)
        return;
    thread_state_ScanNetwork = true;

    int status;
    uint8_t bit;
    if (!modbus || !_gIsConnected) {
        std::cerr << "Not connected. Can not scan" << std::endl;
        thread_state_ScanNetwork = false;
        return;
    }
    for (int address = 1; (address <= MAXIUM_DEVICE_ADDRESS) && thread_state_ScanNetwork; address++) {
        //give other threads time to access modbus
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        modbus_lock.lock();
        modbus_flush(modbus);
        std::cerr << "Pinging <" << address << ">" <<std::endl;
        if (modbus_set_slave(modbus, address) == -1) {
            std::cerr << "Invalid slave ID" << std::endl;
            thread_state_ScanNetwork = false;
            modbus_lock.unlock();
            return;
        }
        status = modbus_read_bits(modbus, 1, 1, &bit);
        modbus_lock.unlock();
        int e = errno;
        switch(status){
            case -1:
                if ( e == 0 ){ // check errno for timeout, device not online
                    //check GetLastError(). error #5 == USB has been unpluged!!!
                    break;
                }
                // device returned an error code, device is online. log it.
            case 1:
                // device online we recived the bits or an error code
                cModbus_Device a;
                a.SetAddress(address);
                a.SetName( std::to_string(address).c_str() );
                auto found = std::find(_gDevice_List.begin(), _gDevice_List.end(), a);
                if(found == _gDevice_List.end() ){
                    std::cerr << "New Device Found <" << address << ">" << std::endl;
                    _gDevice_List.push_back(std::move(a));
                    BuildDeviceList();
                }
            break;
        }
    }
    thread_state_ScanNetwork = false;
}
