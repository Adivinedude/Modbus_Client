#include "cModbus_Template_Manager.h"
#include <fstream>
#include <sstream>

cTemplate_Database_Manager  _gManager;  //template manager

const char* sNameDatabase::GetTemplateValue(uint32_t address, cModbus_Device *pDevice) const
{
    const char* rt = 0;
    uint32_t value;
    uint32_t maskedAddress;
    if( !pDevice )
        throw std::exception("Invalid pDevice in GetTemplateValue()");
    // mask the address register bits so it is compatable with the pDevice addressing scheme
    maskedAddress = address & TEMPLATE_ADDRESS_MASK; 
    
    // search the database for the register we want
    t_name_pair pair = content.equal_range( address );
    
    for (t_name_iter it = pair.first; it != pair.second; it++) {
        // if rt has not been assigned and this is a default value. (without conditions)
        if( (!rt) && it->conditional_address == TEMPLATE_INVALID_ADDRESS){
            if( !it->pValue ) 
                throw std::exception("INVALID DEFAULT pValue in GetTemplateValue()");
            rt = it->pValue->c_str();
        }
        try
        {   // if this is a conditional value, test each condition
            if (it->conditional_address != TEMPLATE_INVALID_ADDRESS) {
                uint32_t masked_conditional_address = it->conditional_address & TEMPLATE_ADDRESS_MASK;
                // retrive the value from the correct register
                switch (it->conditional_address & TEMPLATE_REGISTER_MASK) {
                    case TEMPLATE_ADDRESS_COIL:
                        value = pDevice->GetCoil(masked_conditional_address);
                        break;
                    case TEMPLATE_ADDRESS_DISCRETE_INPUT:
                        value = pDevice->GetDiscreteInput (masked_conditional_address);
                        break;
                    case TEMPLATE_ADDRESS_HOLDING_REGISTER:
                        value = pDevice->GetHoldingRegister(masked_conditional_address);
                        break;
                    case TEMPLATE_ADDRESS_INPUT_REGISTER:
                        value = pDevice->GetInputRegister(masked_conditional_address);
                        break;
                    default:
                        throw std::exception("INVALID TEMPLATE REGISTER in GetTemplateValue()");
                }
                // test the value against the condition
                if( value == it->conditional_value){
                    if (!it->pValue) 
                        throw std::exception("INVALID pValue in GetTemplateValue()");
                    rt = it->pValue->c_str();    // assign value to the return pointer
                    break;
                }
            }
        }
        catch (const std::invalid_argument) {
            continue;
        }
    }  
    return rt;
}

bool sNameDatabase::IsConditionalAddress(uint32_t address) const
{
    std::pair<std::set<uint32_t>::iterator, std::set<uint32_t>::iterator> pair;
    pair = conditional_addresses.equal_range(address);
    return (pair.first == conditional_addresses.end())? false : true;
}

uint32_t cTemplate_Database_Manager::LoadFile(const char* filename)
{
    std::ifstream       file;                   //open file
    std::string         line;                   //current line of text

    std::stringstream   progress_report;        //text output for warnings and errors
    size_t              line_number = 0;        //current line number
    bool                in_conditional = false; //Have we entered into a conditional value
    sNameDatabaseItem   item;
    sNameDatabase*      pDatabase;

    file.open(filename, std::fstream::in);
    if (!file.is_open()) {
        throw std::invalid_argument("Could not open template file");
    }
    pDatabase = new sNameDatabase;
    try
    {
        while (std::getline(file, line)) {
            line_number++;
            //skip empty lines
            if (line.empty())
                continue;
            //skip lines containing only white spaces
            if (line.find_first_not_of(" ") == std::string::npos)
                continue;
            std::istringstream input(line);
            std::string token;

            //skip leading white spaces
            input >> std::ws;

            //check for function characters
            switch (input.peek()) {
                // comments
                // format  " # add your commints here"
            case '#':
                continue;

                // conditional values
                // format  "@ 0x0000 c 0x0000" - '@ address register_type value'
            case '@':

                switch (in_conditional) {
                    //entering conditional statement
                case false:
                    //remove the '@' char
                    input.get();
                    try
                    {
                        // get the address
                        input >> std::hex >> item.conditional_address;
                    }
                    catch (const std::invalid_argument) {
                        progress_report << "Line #" << line_number << " Error: Invalid Address after \'@\'\n";
                        continue;
                    }

                    // get the register_type
                    input >> token;
                    switch (std::tolower(token.at(0))) {
                    case 'c':
                        item.conditional_address |= TEMPLATE_ADDRESS_COIL;
                        break;
                    case 'd':
                        item.conditional_address |= TEMPLATE_ADDRESS_DISCRETE_INPUT;
                        break;
                    case 'h':
                        item.conditional_address |= TEMPLATE_ADDRESS_HOLDING_REGISTER;
                        break;
                    case 'i':
                        item.conditional_address |= TEMPLATE_ADDRESS_INPUT_REGISTER;
                        break;
                    default:
                        progress_report << "Line #" << line_number << " Error: Invalid Register Type. Valid types are c,d,h,i\n";
                        continue;
                    }
                    try
                    {
                        //get the value
                        input >> std::hex >> item.conditional_value;
                        in_conditional = true;
                        pDatabase->conditional_addresses.emplace(item.conditional_address);
                        continue;
                    }
                    catch (const std::invalid_argument) {
                        progress_report << "Line #" << line_number << " Error: Invalid Register Value\n";
                        continue;
                    }
                    break;
                    //exit conditional statement
                case true:
                    in_conditional = false;
                    if (input.gcount())
                        progress_report << "Line #" << line_number << " Warning: Unexpected text after conditional termination.\n";
                    continue;
                    break;
                }
            default:
                //no function characters found
                // format " 0x00 c <string> " 'address register_type value'
                try
                {
                    // get the address
                    input >> std::hex >> item.address;
                    //item.address = std::stoul(token);
                }
                catch (const std::invalid_argument) {
                    progress_report << "Line #" << line_number << " Error: Invalid Address \n";
                    continue;
                }
                // get the register_type
                input >> token;
                switch (std::tolower(token.at(0))) {
                case 'c':
                    item.address |= TEMPLATE_ADDRESS_COIL;
                    break;
                case 'd':
                    item.address |= TEMPLATE_ADDRESS_DISCRETE_INPUT;
                    break;
                case 'h':
                    item.address |= TEMPLATE_ADDRESS_HOLDING_REGISTER;
                    break;
                case 'i':
                    item.address |= TEMPLATE_ADDRESS_INPUT_REGISTER;
                    break;
                default:
                    progress_report << "Line #" << line_number << " Error: Invalid Register Type. Valid types are c,d,h,i\n";
                    continue;
                }
                // get some memory
                item.pValue = new std::string();
                // retreve the input, removing comments
                std::getline(input, token, '#');
                // store the <name> without leading or trailing whitespaces
                *item.pValue = token.substr(token.find_first_not_of(" "), token.find_last_not_of(" "));
                // warning check
                if (item.pValue->empty())
                    progress_report << "Line #" << line_number << " Warning: Empty value\n";
                // allign type of address
                if (!in_conditional)
                    item.conditional_address = TEMPLATE_INVALID_ADDRESS;
                // store the item.
                pDatabase->content.emplace(std::move(item));
            }
        }
        file.close();
        pDatabase->id_number = this->Template_counter++;
        pDatabase->name = filename;
        //search exiting databases for matching filename and delete it.
        for (t_template_iter it = Template_list.begin(); it != Template_list.end(); it++) {
            if ((*it)->name == filename) {
                delete* it;
                Template_list.erase(it);
                break;
            }
        }
        //add new database
        Template_list.push_back(std::move(pDatabase));
    }
    catch (const std::invalid_argument& ex) {
        progress_report << "Unknown Error has occured\n";
        delete pDatabase;
        throw ex;
    }
    return 0;
}

t_template_ptr cTemplate_Database_Manager::GetTemplate(uint32_t template_number)
{
    for (t_template_iter it = Template_list.begin(); it != Template_list.end(); it++) {
        if ((*it)->id_number == template_number) {
            return *it;
        }
    }
    return 0;
}

uint32_t cTemplate_Database_Manager::GetTemplate(const char* template_name)
{
    for (t_template_iter it = Template_list.begin(); it != Template_list.end(); it++) {
        if ((*it)->name == template_name) {
            return (*it)->id_number;
        }
    }
    return 0;
}
