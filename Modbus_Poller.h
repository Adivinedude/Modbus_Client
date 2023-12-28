#ifndef MODBUS_POLLER_H
#define MODBUS_POLLER_H
#include "modbus.h"

struct sConnection_Options {
    int baudrate; 
    char parity; 
    int data_bit; 
    int stop_bit;
};
enum connection_options_enum {
    baud = 0,
    parity,
    data_bit,
    stop_bit
};

extern modbus_t* volatile modbus;
extern volatile sConnection_Options Current_Connection_Settings;
extern volatile bool thread_state_Poller;
extern volatile bool thread_state_ScanNetwork;
extern volatile bool thread_state_ScanDevice;

void connect_modbus(const char* name);
void disconnect_modbus();

void start_poller();
void stop_poller();

void start_ScanDevice();
void stop_ScanDevice();

void start_ScanNetwork();
void stop_ScanNetwork();

void lock_device_list();
void unlock_device_list();
#endif
