#ifndef DEVICEDETAILDISPLAY_H
#define DEVICEDETAILDISPLAY_H
#include <stdint.h>
#include "tui.h"
#include "cModbus_Device.h"

//device list
void SetupBodyWindow();
void OnSelectDevice(const uint16_t value);
void OnClickDevice(const uint16_t value);
void BuildDeviceList();

void cleanDeviceDetailsMenus();
void cleanDeviceDetailsWindow();

void paintDeviceDetails();
void buildDeviceDetail(uint16_t current_selection);
void handleDeviceDetail();
extern WINDOW* pDeviceDetailsWindow;
extern menu_state sDetailList;
extern menu_state sDetailDisplay[];
extern std::list<cModbus_Device>::iterator current_device;
#endif