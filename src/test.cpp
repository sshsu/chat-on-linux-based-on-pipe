#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;/*
int main(){
    string filePath="../config/serverConfig";
    boost::property_tree::ptree root;
    boost::property_tree::ptree items;
   cout<<"xxx\n"; 
    boost::property_tree::read_json<boost::property_tree::ptree>(filePath,root);
   cout<<root.get<string>("SERVER_REG") <<endl;

}*/
