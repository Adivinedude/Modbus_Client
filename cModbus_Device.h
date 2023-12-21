#ifndef CMODBUS_DEVICE_H
#define CMODBUS_DEVICE_H
#include <string>
#include <stdexcept>
#include "cModbus_Template_types.h"

	class cModbus_Device
	{
	public:										    
		cModbus_Device();
		~cModbus_Device();

		std::string template_name;		//just throwing this in here ToDo: do it proper with divised class.
		uint32_t	template_id;
		long long	last_update_time;
		/*
		bool		update_coils;
		bool		update_discrete_input;
		bool		update_holding_register;
		bool		update_input_registers;
		*/
		void force_update(){/*update_coils = 1; update_discrete_input = 1; update_holding_register = 1; update_input_registers = 1; */};

		bool		IsConditionalAddress(uint32_t address);
		//Setup functions
		void configureCoil				(const char* starting_address, const char* size);		//Takes strings as inputs: assocoates memory
		void configureCoil				(size_t starting_address, size_t size);					//Takes intgers as inputs: assocoates memory
		void configureCoil				(size_t starting_address, uint8_t* pBuf, size_t size);	//Takes intgers as inputs: does not assocate memory

		void configureDiscrete_input	(const char* starting_address, const char* size);
		void configureDiscrete_input	(size_t starting_address, size_t size);
		void configureDiscrete_input	(size_t starting_address, uint8_t* pBuf, size_t size);

		void configureHolding_register	(const char* starting_address, const char* size);
		void configureHolding_register	(size_t starting_address, size_t size);
		void configureHolding_register	(size_t starting_address, uint16_t* pBuf, size_t size);

		void configureInput_register	(const char* starting_address, const char* size);
		void configureInput_register	(size_t starting_address, size_t size);
		void configureInput_register	(size_t starting_address, uint16_t* pBuf, size_t size);

		//Member Access Methods
		uint8_t*	GetBufferCoil()									{ return pCoils; }
		uint8_t*	GetBufferDiscreteInput()						{ return pDiscrete_input; }
		uint16_t*	GetBufferHoldingRegister()						{ return pHolding_registers; }
		uint16_t*	GetBufferInputRegister()						{ return pInput_registers; }

		uint16_t	GetAddress()									{ return ModbusAddress; }
		void		SetAddress(uint8_t _address);
		void		SetAddress(const char* address);
		void		SetAddress(uint16_t, const char* address)		{ SetAddress(address); }

		const char* GetName()										{return name.c_str(); }
		void		SetName(const char* _name);
		void		SetName(uint16_t address, const char* _name)	{ SetName(_name); }	//wrapper method

		uint16_t	GetCoilN()										{ return nCoils; }
		uint16_t	GetStartCoil()									{ return start_address_coils; }
		uint16_t	GetCoil(uint16_t address);
		void		SetCoil(uint16_t address, bool value);
		void		SetCoil(uint16_t address, const char* value);

		uint16_t	GetDiscreteInputN()								{ return nDiscrete_input; }
		uint16_t	GetStartDiscreteInput()							{ return start_address_discrete_input; }
		uint16_t	GetDiscreteInput(uint16_t address);
		void		SetDiscreteInput(uint16_t address, uint16_t value);

		uint16_t	GetHoldingRegisterN()							{ return nHolding_registers; }
		uint16_t	GetStartHoldingRegister()						{ return start_address_holding_registers; }
		uint16_t	GetHoldingRegister(uint16_t address);
		void		SetHoldingRegister(uint16_t address, uint16_t value);
		void		SetHoldingRegister(uint16_t address, const char* value);

		uint16_t	GetStartInputRegister()							{ return start_address_input_registers; }
		uint16_t	GetInputRegisterN()								{ return nInput_registers; }
		uint16_t	GetInputRegister(uint16_t address);
		void		SetInputRegister(uint16_t address, uint16_t value);

		T_Options	GetDefaultFormatOptions();
		std::string MakeFormattedString					(const char* cPrefix, const uint16_t value, const T_Options* options = 0);
		std::string MakeFormattedString					(const char* cPrefix, const char* cSuffix, const T_Options* options = 0);
		std::string GetFormattedString_Name				(uint16_t, const T_Options* =0);
		std::string GetFormattedString_Address			(uint16_t, const T_Options* =0);

		//Template string routine
		const char* GetFormattedTemplate_String			(uint32_t address, uint32_t register_type);
		std::string GetFormattedString					(uint16_t, uint32_t, uint16_t(cModbus_Device::*)(uint16_t), const T_Options*);
		std::string GetFormattedString_Coil				(uint16_t, const T_Options* =0);
		std::string GetFormattedString_DiscreteInput	(uint16_t, const T_Options* =0);
		std::string GetFormattedString_HoldingRegister	(uint16_t, const T_Options* =0);
		std::string GetFormattedString_InputRegister	(uint16_t, const T_Options* =0);
		
		void ReadAll();	//Read all registers from the server
		void WriteAll();//Write all registers to the server

		bool operator == (const cModbus_Device &a)	{return ( a.name == name || a.ModbusAddress == ModbusAddress);}	//use std::find
		bool operator == (const uint16_t& c)		{ return ModbusAddress == c; }									//use std::find 
		bool operator <  (const cModbus_Device& a) { return (ModbusAddress < a.ModbusAddress); }					//use std::list.sort
		bool operator >  (const cModbus_Device& a) { return (ModbusAddress > a.ModbusAddress); }					//use std::list.sort

	private:
		//Cleanup Methods
		void clear_coils();
		void clear_discrete_inputs();
		void clear_holding_registers();
		void clear_input_registers();

		//Error Checking
		void error_check_config(size_t starting_address, size_t size);
		void error_check_access(void* p, size_t starting_address, size_t size, uint16_t address);

		//Members
		std::string name;
		uint16_t	ModbusAddress;					//Modbus Address

		uint8_t*	pCoils;							//pointer to coil buffer
		uint16_t	nCoils;							//number of coils
		uint16_t	start_address_coils;			//starting address of registers
		bool		clean_coils;					//run cleanup code of buffer?

		uint8_t*	pDiscrete_input;				//pointer to descrete input buffer
		uint16_t	nDiscrete_input;				//number of descrete input
		uint16_t	start_address_discrete_input;
		bool		clean_discrete_input;

		uint16_t*	pHolding_registers;				//pointer to holding registers buffer
		uint16_t	nHolding_registers;				//number of holding registers
		uint16_t	start_address_holding_registers;
		bool		clean_holding_registers;

		uint16_t*	pInput_registers;				//pointer to input registers buffer
		uint16_t	nInput_registers;				//number of input registers
		uint16_t	start_address_input_registers;
		bool		clean_input_registers;

		T_Options	default_options;
	};
#endif
