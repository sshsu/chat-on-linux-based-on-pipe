//
// Created by woder on 1/2/18.
//

#include "jsonOpera.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


bool json_add(char* others,char* key,char* value){
    boost::property_tree::ptree root;
    std::stringstream s (others);
    try {
        read_json(s, root); //读取
    }
    catch(std::exception & e){
        printf("cannot parse from string 'others' \n");
        return false;
    }

    root.put(key,value);
    write_json(s, root,false);
    memset(others, 0 ,BUF_LEN* sizeof(char));
    memcpy(others,s.str().data(), sizeof(char) * strlen(s.str().data()));
    return true;
}

bool json_get(char* buf,int buf_size ,char* others,char*key){

    boost::property_tree::ptree root;
    std::stringstream s (others);

    try {
        read_json(s, root); //读取
        memset(buf, 0 ,sizeof(char) * buf_size);
        memcpy(buf, root.get<std::string>(key).data(), sizeof(char) * strlen(root.get<std::string>(key).data()));
    }
    catch(std::exception & e){
        printf("cannot parse from info struct,error is %s \n",e.what());
        return false;
    }
    return true;
}