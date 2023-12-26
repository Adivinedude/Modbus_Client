#ifndef CMODBUS_TEMPLATE_TYPES_H
#define CMODBUS_TEMPLATE_TYPES_H
#include <string>

typedef struct sTemplateTextFormatOptions {
    uint8_t         cell_size;
    uint8_t         allign_prefix;
    //uint8_t         allign_root;
    uint8_t         allign_suffix;
    const char*     root_string;
    const char*     suffix_prepend;
    uint8_t         suffix_width;
    char            suffix_fill;
    std::ios_base& (*manipulator)(std::ios_base&);
} T_Options;

enum TEMPLATE_ALLIGN_OPTIONS {
    TEMPLATE_ALLIGN_LEFT = 0,
    TEMPLATE_ALLIGN_CENTER,
    TEMPLATE_ALLIGN_RIGHT
};

#define TEMPLATE_INVALID_ADDRESS -1
enum TEMPLATE_ADDRESS_TYPE
{
    TEMPLATE_ADDRESS_COIL = 0x01000000,
    TEMPLATE_ADDRESS_DISCRETE_INPUT = 0x02000000,
    TEMPLATE_ADDRESS_HOLDING_REGISTER = 0x03000000,
    TEMPLATE_ADDRESS_INPUT_REGISTER = 0x04000000,
    TEMPLATE_ADDRESS_MASK = 0x0000FFFF,
    TEMPLATE_REGISTER_MASK = 0xFFFF0000
};


struct sNameDatabaseItem
{
    uint32_t    address;
    std::string* pValue;
    uint32_t    conditional_address;
    uint16_t    conditional_value;

    sNameDatabaseItem()           :address(TEMPLATE_INVALID_ADDRESS), pValue(0), conditional_address(TEMPLATE_INVALID_ADDRESS), conditional_value(0) {};
    sNameDatabaseItem(uint32_t a) :address(a),                        pValue(0), conditional_address(TEMPLATE_INVALID_ADDRESS), conditional_value(0) {};
    sNameDatabaseItem(sNameDatabaseItem&& r)
    {
        sNameDatabaseItem::sNameDatabaseItem();
        *this = std::move(r);
    }
    sNameDatabaseItem& operator=(sNameDatabaseItem&& r) {
        if( this != &r ){
            address = r.address;
            pValue = std::move(r.pValue);
            conditional_address = r.conditional_address;
            conditional_value = r.conditional_value;
            r.address = TEMPLATE_INVALID_ADDRESS;
            r.pValue = 0;
            r.conditional_address = TEMPLATE_INVALID_ADDRESS;
            r.conditional_value = 0;
        }
        return *this;
    }
      
    ~sNameDatabaseItem() { 
        if (pValue){ 
            delete pValue; 
        }
    }
    private:
        //no copying me.
        sNameDatabaseItem(const sNameDatabaseItem& r)
        : address(r.address),
        conditional_address(r.conditional_address),
        conditional_value(r.conditional_value)
        {
            if (!r.pValue) {
                pValue = 0;
            }
            else {
                pValue = new std::string(*r.pValue);
            }
        }
};

#endif