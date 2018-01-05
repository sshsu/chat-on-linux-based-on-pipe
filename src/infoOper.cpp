//
// Created by woder on 1/3/18.
//


#include "infoOper.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

bool send_info(info& ainfo,char* pipe_name){
    std::cout<<pipe_name<<std::endl;
    if(access(pipe_name,F_OK) == 0){//如果用户文件存在
        int dest_fd = open(pipe_name , O_WRONLY );//打开文件
        if(dest_fd < 0){
            printf("cannot open file,error is %s\n",strerror(errno));
            return false;
        }

        send_info(ainfo,dest_fd);
        close(dest_fd);
        return true;
    }
    else{
        printf("file %s doesn't exit, cannot send_info\n",pipe_name);
    }
    return false;
}

bool send_info(info& ainfo, int dest_fd){
    unsigned int len = 0 ;
    int ret;
    while(1){
        ret = write( dest_fd, (void*)(&ainfo)+len,sizeof(ainfo)-len);
       // std::cout<<len<<std::endl;
        if(ret == -1)
            continue;
        len +=ret;
        if(len == sizeof(ainfo))
            break;

    }
    return true;
}



bool read_info(info& ainfo, int dest_fd){
    unsigned int len = 0 ;
    int ret;
    while(1){
        ret =read(dest_fd, (void*)(&ainfo)+len,sizeof(ainfo) -len);
        if( ret == -1)
            continue;
        len = len + ret;
        if(len == sizeof(ainfo))
            return true;
    }

}

bool read_info(info& ainfo, char* pipe_name){
    if(access(pipe_name,F_OK) == 0){//如果用户文件存在
        int dest_fd = open(pipe_name , O_RDONLY|O_NONBLOCK );//以阻塞写形式打开，如果没有阻塞读，则一直阻塞
        if(dest_fd < 0){
            printf("cannot open file,error is %s\n",strerror(errno));
            return false;
        }
        read_info(ainfo,dest_fd);
        close(dest_fd);
        return true;
    }else{
        printf("file %s doesn't exit, cannot read_info\n",pipe_name);
    }
    return false;

}