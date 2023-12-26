#include "cModbus_Device.h"
#include "modbus.h"
#include "string.h"
#include <sstream>
#include <iomanip>
#include "cModbus_Template_Manager.h"
cModbus_Device::cModbus_Device(){
	template_id = 0;
	last_update_time = 0;
	ModbusAddress = 0xFFFF;		//Modbus Address

	pCoils = 0;				//pointer to coil buffer
	nCoils = 0;				//number of coils
	start_address_coils = 0;
	clean_coils = 0;		//run cleanup code of buffer?
	//update_coils = 0;

	pDiscrete_input = 0;	//pointer to descrete input buffer
	nDiscrete_input = 0;	//number of descrete input
	start_address_discrete_input = 0;
	clean_discrete_input = 0;
	//update_discrete_input = 0;

	pHolding_registers = 0;	//pointer to holding registers buffer
	nHolding_registers = 0;	//number of holding registers
	start_address_holding_registers = 0;
	clean_holding_registers = 0;
	//update_holding_register = 0;

	pInput_registers = 0;	//pointer to input registers buffer
	nInput_registers = 0;	//number of input registers
	start_address_input_registers = 0;
	clean_input_registers = 0;
	//update_input_registers = 0;

	memset(&default_options, 0, sizeof(default_options));
	default_options.cell_size		= 17;
	default_options.allign_prefix	= TEMPLATE_ALLIGN_LEFT;
	//default_options.allign_root		= TEMPLATE_ALLIGN_CENTER;
	default_options.allign_suffix	= TEMPLATE_ALLIGN_RIGHT;
	default_options.root_string		= ":";
	//default_options.suffix_width	= 4;
	//default_options.suffix_prepend	= "0x";
	//default_options.suffix_fill		= '0';
	default_options.manipulator		= std::dec;
}
cModbus_Device::~cModbus_Device() {
	clear_coils();
	clear_discrete_inputs();
	clear_holding_registers();
	clear_input_registers();
}
cModbus_Device::cModbus_Device(cModbus_Device&& r)
{
	//minimial construction 
	this->pCoils = 0;
	this->pDiscrete_input = 0;
	this->pHolding_registers = 0;
	this->pInput_registers = 0;
	*this = std::move(r);
}
cModbus_Device& cModbus_Device::operator=(cModbus_Device&& r)
{
	if( this != &r ){
		this->clear_coils();
		this->clear_discrete_inputs();
		this->clear_holding_registers();
		this->clear_input_registers();

		template_id = r.template_id;
		template_name = std::move(r.template_name);
		last_update_time = r.last_update_time;

		name = std::move(r.name);
		ModbusAddress = r.ModbusAddress;

		pCoils = r.pCoils;
		nCoils = r.nCoils;
		start_address_coils = r.start_address_coils;
		clean_coils = r.clean_coils;

		pDiscrete_input = r.pDiscrete_input;
		nDiscrete_input = r.nDiscrete_input;
		start_address_discrete_input = r.start_address_discrete_input;
		clean_discrete_input = r.clean_discrete_input;

		pHolding_registers = r.pHolding_registers;
		nHolding_registers = r.nHolding_registers;
		start_address_holding_registers = r.start_address_holding_registers;
		clean_holding_registers = r.clean_holding_registers;

		pInput_registers = r.pInput_registers;
		nInput_registers = r.nInput_registers;
		start_address_input_registers = r.start_address_input_registers;
		clean_input_registers = r.clean_input_registers;

		default_options = r.default_options;

		r.ModbusAddress = 0xFFFF;
		r.clean_coils = 0;
		r.clean_discrete_input = 0;
		r.clean_holding_registers = 0;
		r.clean_input_registers = 0;

		r.pCoils = 0;
		r.pDiscrete_input = 0;
		r.pHolding_registers = 0;
		r.pInput_registers = 0;

		r.nCoils = 0;
		r.nDiscrete_input = 0;
		r.nHolding_registers = 0;
		r.nInput_registers = 0;
	}
	return *this;
}



bool cModbus_Device::IsConditionalAddress(uint32_t address)
{
	t_template_ptr pDb;
	//first attempt to get the database
	pDb = _gManager.GetTemplate(template_id);
	//if the database is outdated/does not exist
	if (!pDb) {
		//retreve id by name
		template_id = _gManager.GetTemplate(template_name.c_str());
		pDb = _gManager.GetTemplate(template_id);
	}
	// if we successfully retrived the database 
	if (pDb) {
		//Get a pointer to the template value
		return pDb->IsConditionalAddress(address);
	}
	return false;
}

////////// Clean up methods //////////
void cModbus_Device::clear_coils() {
	if (clean_coils) {
		if (nCoils && pCoils) {
			delete[] pCoils;
			nCoils = 0;
			pCoils = 0;
			clean_coils = 0;
		}
	}
}
void cModbus_Device::clear_discrete_inputs() {
	if (clean_discrete_input) {
		if (nDiscrete_input && pDiscrete_input) {
			delete[] pDiscrete_input;
			nDiscrete_input = 0;
			pDiscrete_input = 0;
			clean_discrete_input = 0;
		}
	}
}
void cModbus_Device::clear_holding_registers() {
	if (clean_holding_registers) {
		if (nHolding_registers && pHolding_registers) {
			delete[] pHolding_registers;
			nHolding_registers = 0;
			pHolding_registers = 0;
			clean_holding_registers = 0;
		}
	}
}
void cModbus_Device::clear_input_registers(){
	if (clean_input_registers) {
		if (nInput_registers && pInput_registers) {
			delete[] pInput_registers;
			nInput_registers = 0;
			pInput_registers = 0;
			clean_input_registers = 0;
		}
	}
}

////////// Error Checking //////////
void cModbus_Device::error_check_config(size_t starting_address, size_t size) {
	if (starting_address > 0x0000FFFF)
		throw std::invalid_argument("Invalid Starting Address");
	if (size > 0x0000FFFF)
		throw std::invalid_argument("Invalid Size");
	// if the starting address+size excedes address space available
	if (0x0000FFFF - starting_address + 1 < size) {
		throw std::invalid_argument("Invalid size and address combination");
	}
}
void cModbus_Device::error_check_access(void* p, size_t starting_address, size_t size, uint16_t address) {
	if (!p || !size)
		throw std::invalid_argument("Data Not Initalized Yet");
	if ( (address < starting_address)
		||
		(starting_address + size < address)
		){
			std::stringstream st;
			st << "cModbus_Device:Access Error <" << std::dec << GetAddress() << ":" << std::hex << address
				<< ">" << "Address out of range";
			throw std::invalid_argument(st.str());
		}
}

////////// config methods	//////////
//coils
void cModbus_Device::configureCoil(const char* starting_address, const char* size) {
	size_t address = 0xFFFFFFFF, s;
	try {
		address = strlen(starting_address) ? std::stoul(starting_address, nullptr, 10) : 0;
		if (!strlen(size))
			return;
		s = std::stoul(size, nullptr, 10);
	}
	catch (std::invalid_argument) 
	{	
		if( address == 0xFFFFFFFF)
			throw std::invalid_argument("Invalid Coil Address");
		throw std::invalid_argument("Invalid Coil Count");
	}

	configureCoil(address, s);
}
void cModbus_Device::configureCoil(size_t starting_address, size_t size)
{
	uint8_t* p = 0;
	if (!size)
		return;
	try {
		p = new uint8_t[size];
		memset(p, 0, sizeof(uint8_t) * size);
		configureCoil(starting_address, p, size);
		clean_coils = 1;
	}
	catch (std::invalid_argument const& ex)
	{
		delete[] p;
		throw ex;
	}
}
void cModbus_Device::configureCoil(size_t starting_address, uint8_t* pBuf, size_t size)
{
	clear_coils();
	error_check_config(starting_address, size);
	start_address_coils = (uint16_t)starting_address;
	pCoils = pBuf;
	nCoils = (uint16_t)size;
}

//Discrete Input
void cModbus_Device::configureDiscrete_input(const char* starting_address, const char* size) {
	size_t address = 0xFFFFFFFF, s;
	try {
		address = strlen(starting_address) ? std::stoul(starting_address, nullptr, 10) : 0;
		if (!strlen(size))
			return;
		s = std::stoul(size, nullptr, 10);
	}
	catch (std::invalid_argument)
	{
		if (address == 0xFFFFFFFF)
			throw std::invalid_argument("Invalid Coil Address");
		throw std::invalid_argument("Invalid Coil Count");
	}
	configureDiscrete_input(address, s);
}
void cModbus_Device::configureDiscrete_input(size_t starting_address, size_t size)
{
	uint8_t* p = 0;
	if (!size)
		return;
	try {
		p = new uint8_t[size];
		memset(p, 0, sizeof(uint8_t) * size);
		configureDiscrete_input(starting_address, p, size);
		clean_discrete_input = 1;
	}
	catch (std::invalid_argument const& ex)
	{
		delete[] p;
		throw ex;
	}
}
void cModbus_Device::configureDiscrete_input(size_t starting_address, uint8_t* pBuf, size_t size)
{
	clear_discrete_inputs();
	error_check_config(starting_address, size);
	start_address_discrete_input = (uint16_t)starting_address;
	pDiscrete_input = pBuf;
	nDiscrete_input = (uint16_t)size;
}

//Holding Register
void cModbus_Device::configureHolding_register(const char* starting_address, const char* size) {
	size_t address = 0xFFFFFFFF, s;
	try {
		address = strlen(starting_address) ? std::stoul(starting_address, nullptr, 10) : 0;
		if (!strlen(size))
			return;
		s = std::stoul(size, nullptr, 10);
	}
	catch (std::invalid_argument)
	{
		if (address == 0xFFFFFFFF)
			throw std::invalid_argument("Invalid Coil Address");
		throw std::invalid_argument("Invalid Coil Count");
	}
	configureHolding_register(address, s);
}
void cModbus_Device::configureHolding_register(size_t starting_address, size_t size)
{
	uint16_t* p = 0;
	if (!size)
		return;
	try {
		p = new uint16_t[size];
		memset(p, 0, sizeof(uint16_t) * size);
		configureHolding_register(starting_address, p, size);
		clean_holding_registers = 1;
	}
	catch (std::invalid_argument const& ex)
	{
		delete[] p;
		throw ex;
	}
}
void cModbus_Device::configureHolding_register(size_t starting_address, uint16_t* pBuf, size_t size)
{
	clear_holding_registers();
	error_check_config(starting_address, size);
	start_address_holding_registers = (uint16_t)starting_address;
	pHolding_registers = pBuf;
	nHolding_registers = (uint16_t)size;
}

//Input Register
void cModbus_Device::configureInput_register(const char* starting_address, const char* size) {
	size_t address = 0xFFFFFFFF, s;
	try {
		address = strlen(starting_address) ? std::stoul(starting_address, nullptr, 10) : 0;
		if (!strlen(size))
			return;
		s = std::stoul(size, nullptr, 10);
	}
	catch (std::invalid_argument)
	{
		if (address == 0xFFFFFFFF)
			throw std::invalid_argument("Invalid Coil Address");
		throw std::invalid_argument("Invalid Coil Count");
	}
	configureInput_register(address, s);
}
void cModbus_Device::configureInput_register(size_t starting_address, size_t size)
{
	uint16_t* p = 0;
	if (!size)
		return;
	try {
		p = new uint16_t[size];
		memset(p, 0, sizeof(uint16_t) * size);
		configureInput_register(starting_address, p, size);
		clean_input_registers = 1;
	}
	catch (std::invalid_argument const& ex)
	{
		delete[] p;
		throw ex;
	}
}
void cModbus_Device::configureInput_register(size_t starting_address, uint16_t* pBuf, size_t size)
{
	clear_input_registers();
	error_check_config(starting_address, size);
	start_address_input_registers = (uint16_t)starting_address;
	pInput_registers = pBuf;
	nInput_registers = (uint16_t)size;
}

/////////// Member Access Methods //////////
//Address
void		cModbus_Device::SetAddress(uint8_t address)
{
    this->ModbusAddress = address;
}
void		cModbus_Device::SetAddress(const char* _address)
{
	try {
		unsigned long a = std::stoul(_address, nullptr, 10);
		if (a > 255)
			throw std::invalid_argument("");
		this->SetAddress((uint8_t)a);
	}
	catch (std::invalid_argument ) {
		throw std::invalid_argument("Invalid Modbus Address");
	}
}

//Name
void		cModbus_Device::SetName(const char* _name)
{
	if ( !_name || !strlen(_name))
		throw std::invalid_argument("Invalid Name");
    this->name = _name;
}

//Coils
uint16_t	cModbus_Device::GetCoil(uint16_t address)
{
	error_check_access(pCoils, start_address_coils, nCoils, address);
	return pCoils[address - start_address_coils];
}
void		cModbus_Device::SetCoil(uint16_t address, bool value)
{
	error_check_access(pCoils, start_address_coils, nCoils, address);
	//if( pCoils[address - start_address_coils]?1:0 != value){
		pCoils[address - start_address_coils] = value;
	//	update_coils = true;
	//}
}
void		cModbus_Device::SetCoil(uint16_t address, const char* value) {
	unsigned long v;
	if (strstr(value, "on")){
		v = 1;
	}else if(strstr(value, "off")) {
		v = 0;
	}else {
		v = std::stoul(value, nullptr, 10);
	}
	this->SetCoil(address, v > 0);
}

//Discrete Input
uint16_t	cModbus_Device::GetDiscreteInput(uint16_t address)
{
	error_check_access(pDiscrete_input, start_address_discrete_input, nDiscrete_input, address);
	return pDiscrete_input[address - start_address_discrete_input];
}
void		cModbus_Device::SetDiscreteInput(uint16_t address, uint16_t value)
{
	error_check_access(pDiscrete_input, start_address_discrete_input, nDiscrete_input, address);
	//if(pDiscrete_input[address - start_address_discrete_input] != value ? 1 : 0){
		pDiscrete_input[address - start_address_discrete_input] = value?1:0;
	//	update_input_registers = true;
	//}
}

//Holding Register
uint16_t	cModbus_Device::GetHoldingRegister(uint16_t address)
{
	error_check_access(pHolding_registers, start_address_holding_registers, nHolding_registers, address);
	return pHolding_registers[address - start_address_holding_registers];
}
void		cModbus_Device::SetHoldingRegister(uint16_t address, uint16_t value)
{
	error_check_access(pHolding_registers, start_address_holding_registers, nHolding_registers, address);
	//if(pHolding_registers[address - start_address_holding_registers] != value){
		pHolding_registers[address - start_address_holding_registers] = value;
	//	update_holding_register = true;
	//}
}
void		cModbus_Device::SetHoldingRegister(uint16_t address, const char* value) {
	unsigned long v = std::stoul(value, nullptr, 10);
	if (v > 0xFFFF)
		throw std::invalid_argument("Invalid Holding Register Value");
	this->SetHoldingRegister(address, (uint16_t)v);
}

//Input Register
uint16_t	cModbus_Device::GetInputRegister(uint16_t address)
{
	error_check_access(pInput_registers, start_address_input_registers, nInput_registers, address);
	return pInput_registers[address - start_address_input_registers];
}
void		cModbus_Device::SetInputRegister(uint16_t address, uint16_t value)
{
	error_check_access(pInput_registers, start_address_input_registers, nInput_registers, address);
	//if(pInput_registers[address - start_address_input_registers] != value){
		pInput_registers[address - start_address_input_registers] = value;
	//	update_input_registers = true;
	//}
}

////////// Make Formatted String //////////
T_Options	cModbus_Device::GetDefaultFormatOptions() { 
	return default_options; 
}
std::string cModbus_Device::MakeFormattedString(const char* cPrefix, const uint16_t value, const T_Options* options)
{
	std::stringstream st;
	if (!options)
		options = &default_options;

	//make the suffix string
	if (options->suffix_prepend)
		st << options->suffix_prepend;
	if (options->manipulator)
		st << (*options->manipulator);
	if (options->suffix_width && options->suffix_fill) {
		st << std::setw(options->suffix_width) << std::setfill(options->suffix_fill);
	}
	st << value;
	return MakeFormattedString( cPrefix, st.str().c_str(), options);	
}
std::string cModbus_Device::MakeFormattedString(const char* cPrefix, const char* cSuffix, const T_Options* options)
{
	std::stringstream st;
	std::string prefix, suffix;
	int total_string_length, root_length;
	if (!options)
		options = &default_options;
	if (cPrefix)
		prefix = cPrefix;
	if (cSuffix)
		suffix = cSuffix;

	root_length = strlen(options->root_string);
	total_string_length = prefix.size() + root_length + suffix.size();

	//is the cell big enough for allignments
	if (options->cell_size > total_string_length) {
		//fill st with whitespace
		for (int i = options->cell_size; i; i--)
			st << ' ';
		st.seekp(std::ios_base::beg);	// go back to the begining

		//allign and print the prefix
		switch (options->allign_prefix) {
		case TEMPLATE_ALLIGN_LEFT:
			break;
		case TEMPLATE_ALLIGN_CENTER:
			st.seekp((options->cell_size / 4) - (prefix.size() / 2) - (prefix.size() / 2));
			break;
		case TEMPLATE_ALLIGN_RIGHT:
			st.seekp((options->cell_size / 2) - prefix.size() - (root_length / 2));
			break;
		default:
			throw std::invalid_argument("Invalid 'allign_prefix' option");
		}
		st << prefix;

//		st.seekp((total_string_length / 2) - (root_length / 2));
		st << options->root_string;

		//print the suffix
		switch (options->allign_suffix) {
		case TEMPLATE_ALLIGN_LEFT:
			break;
		case TEMPLATE_ALLIGN_CENTER:
			st.seekp((options->cell_size / 4 * 3) + (root_length / 2) - (suffix.size() / 2));
			break;
		case TEMPLATE_ALLIGN_RIGHT:
			st.seekp(options->cell_size - suffix.size());
			break;
		default:
			throw std::invalid_argument("Invalid 'allign_suffix' option");
		}
		st << suffix;
		return st.str();
	}
	st << prefix;
	if (options->root_string)
		st << options->root_string;
	st << suffix;
	return st.str();
}

////////// Get Formatted String ///////////
const char* cModbus_Device::GetFormattedTemplate_String(uint32_t address, uint32_t register_type) {
	t_template_ptr pDb;
	//first attempt to get the database
	pDb = _gManager.GetTemplate(template_id);
	//if the database is outdated/does not exist
	if (!pDb) {
		//retreve id by name
		template_id = _gManager.GetTemplate(template_name.c_str());
		pDb = _gManager.GetTemplate(template_id);
	}
	// if we successfully retrived the database 
	if (pDb) {
		//Get a pointer to the template value
		return pDb->GetTemplateValue(address | register_type, this);
	}
	return 0;
}
std::string cModbus_Device::GetFormattedString_Name(uint16_t, const T_Options* options) {
	T_Options t = GetDefaultFormatOptions();
	//t.cell_size = options ? options->cell_size : 15;
	return MakeFormattedString("Name", name.c_str(), options);
}
std::string cModbus_Device::GetFormattedString_Address(uint16_t, const T_Options* options) {
	T_Options t = GetDefaultFormatOptions();
	//t.cell_size			= options?options->cell_size:15;
	t.manipulator		= std::dec;
	t.suffix_prepend	= 0;
	t.suffix_width		= 0;
	t.suffix_fill		= 0;
	return MakeFormattedString("Address", ModbusAddress, &t);
}

std::string cModbus_Device::GetFormattedString(uint16_t address, uint32_t type, uint16_t (cModbus_Device::*method)(uint16_t), const T_Options* options) {
	std::stringstream st;
	std::string prefix;
	const char* template_string = GetFormattedTemplate_String(address, type);
	if (template_string)
		prefix = template_string;
	else {
		st << "0x" << std::hex << std::setw(4) << std::setfill('0') << address;
		prefix = st.str();
		st.str(std::string());
		st.clear();
	}
	switch (type) {
		case TEMPLATE_ADDRESS_COIL:
		case TEMPLATE_ADDRESS_DISCRETE_INPUT:
			return MakeFormattedString(prefix.c_str(), ((*this).*method)(address)?" ON":"OFF", options);
		default:
			return MakeFormattedString(prefix.c_str(), ((*this).*method)(address), options);
	}
}

std::string cModbus_Device::GetFormattedString_Coil(uint16_t address, const T_Options* options) {
	return GetFormattedString(address, TEMPLATE_ADDRESS_COIL, &cModbus_Device::GetCoil, options);
}
std::string cModbus_Device::GetFormattedString_DiscreteInput(uint16_t address, const T_Options* options){
	return GetFormattedString(address, TEMPLATE_ADDRESS_DISCRETE_INPUT, &cModbus_Device::GetDiscreteInput, options);
}

std::string cModbus_Device::GetFormattedString_HoldingRegister(uint16_t address, const T_Options* options) {
	return GetFormattedString(address, TEMPLATE_ADDRESS_HOLDING_REGISTER, &cModbus_Device::GetHoldingRegister, options);
}
std::string cModbus_Device::GetFormattedString_InputRegister(uint16_t address, const T_Options* options) {
	return GetFormattedString(address, TEMPLATE_ADDRESS_INPUT_REGISTER, &cModbus_Device::GetInputRegister, options);
}