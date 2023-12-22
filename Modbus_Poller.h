#ifndef MODBUS_POLLER_H
#define MODBUS_POLLER_H
#include "modbus.h"

struct sConnection_Options {
    int baudrate; 
    char parity; 
    int data_bit; 
    int stop_bit;
};

extern modbus_t* volatile modbus;
extern volatile sConnection_Options Current_Connection_Settings;

void connect_poller(const char* name);
void lock_poller();
void unlock_poller();
void start_poller();
void stop_poller();
void modbus_poller();
void scan_device_for_registers(const uint16_t);
void scan_network_for_devices(const uint16_t);

#endif
