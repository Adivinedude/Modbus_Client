#ifndef MODBUS_POLLER_H
#define MODBUS_POLLER_H
#include "modbus.h"

extern modbus_t* volatile modbus;

void connect_poller(const char* name);

void lock_poller();
void unlock_poller();

void start_poller();
void stop_poller();

void modbus_poller();

void scan_device_for_registers(const uint16_t);
void scan_network_for_devices(const uint16_t);

#endif
