#ifndef DEVICEDETAILDISPLAY_H
#define DEVICEDETAILDISPLAY_H
#include <stdint.h>
#include "tui.h"

void cleanDeviceDetailsMenus();
void cleanDeviceDetailsWindow();

void paintDeviceDetails();
void buildDeviceDetail(uint16_t current_selection);
void handleDeviceDetail();
extern WINDOW* pDeviceDetailsWindow;
#endif