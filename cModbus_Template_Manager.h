#ifndef CMODBUS_TEMPLATE_MANAGER_H
#define CMODBUS_TEMPLATE_MANAGER_H

    #include <string>
    #include <list>
    #include <set>
    #include <ios>
    #include "cModbus_Device.h"
    #include "cModbus_Template_types.h"

    // this template requires the class to contain a member of type <COMPARE_TYPE> named <COMPARE_MEMBER>
    template <class SET_OBJECT, class COMPARE_TYPE, COMPARE_TYPE SET_OBJECT::*MEMBER> class SIMPLE_SET_COMPARATOR
    {
    public:
        using is_transparent = std::true_type;
        bool operator()(const COMPARE_TYPE& left, const SET_OBJECT& right) const { return left < right.*MEMBER; }
        bool operator()(const SET_OBJECT& left, const COMPARE_TYPE& right) const { return left.*MEMBER < right; }
        bool operator()(const SET_OBJECT& left, const SET_OBJECT& right)   const { return left.*MEMBER < right.*MEMBER; }
    };

   
    //inline bool operator < (const sNameDatabaseItem &left, const sNameDatabaseItem &right){return left.address < right.address;}
    
    typedef SIMPLE_SET_COMPARATOR<sNameDatabaseItem, uint32_t, &sNameDatabaseItem::address > t_name_comparator;
    typedef std::multiset<sNameDatabaseItem, t_name_comparator >    t_name_database;
    typedef t_name_database::iterator                               t_name_iter;
    typedef std::pair<t_name_iter,  t_name_iter>                    t_name_pair;

    struct sNameDatabase
    {
        uint32_t            id_number;
        std::string         name;
        t_name_database     content;
        std::set<uint32_t>  conditional_addresses;
        const char*         GetTemplateValue(uint32_t address, cModbus_Device *pDevice) const;
        bool                IsConditionalAddress(uint32_t address) const;
        sNameDatabase(): id_number(TEMPLATE_INVALID_ADDRESS), name(std::string()) {};
    };

    typedef std::list<sNameDatabase*> t_template_list;
    typedef t_template_list::iterator t_template_iter;
    typedef const sNameDatabase*      t_template_ptr;
    class cTemplate_Database_Manager
    {
        t_template_list Template_list;
        uint32_t        Template_counter;
    public:                 
        cTemplate_Database_Manager() :Template_counter(1) {};
        ~cTemplate_Database_Manager(){};
        uint32_t        LoadFile    (const char* filename);
        t_template_ptr  GetTemplate (uint32_t    template_number);
        uint32_t        GetTemplate (const char* template_name);
    };

    extern cTemplate_Database_Manager _gManager;
#endif

