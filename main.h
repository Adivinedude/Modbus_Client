#ifndef MAIN_H
#define MAIN_H
	#include <list>
	#include "tui.h"
	#include "cModbus_Device.h"
	#include "cModbus_Template_Manager.h"

	#define DEFAULT_FIELD_LENGTH 25

	extern std::list<cModbus_Device>	_gDevice_List;
	extern cTemplate_Database_Manager	_gManager;
	
	extern volatile bool				_gIsConnected;

	void BuildDeviceList();
#endif
