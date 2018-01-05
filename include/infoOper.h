//
// Created by woder on 1/3/18.
//

#include "myStruct.h"


#ifndef FINAL_INFOOPER_H
#define FINAL_INFOOPER_H

bool send_info(info& ainfo,char* pipe_name);
bool send_info(info& ainfo, int dest_fd);
bool read_info(info& ainfo, int dest_fd);
bool read_info(info& ainfo, char* pipe_name);
#endif //FINAL_INFOOPER_H
