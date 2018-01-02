//
// Created by woder on 1/1/18.
//

//
// Created by woder on 10/28/17.
//
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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include  <iostream>
#include "myStruct.h"
using namespace std;



int reg_fd,log_fd,chat_fd;  //打开服务器管道后的对应文件描述符
status stus = offline;        //用户的初始状态


/**
 *　由于reg,log和chat三种功能的输入结构都差不多可以分解为A+分割符号+B，所以只需要把这两部分提取出来即可
 * @param input_str 用户输入的内容
 * @param acontent  作为解析后存放的结构
 * @param separator 两段内容的分割符
 * @param end input_str的长度
 * @return 解析提取成功返回true，否则返回false
 */

bool cat_content(char *input_str, content* acontent, char separator) {

    int len = strlen(input_str), index = 0;

    while (index < len && input_str[index] != separator)//找到第一部分内容的结尾
        ++index;

    if( index < len)
        memcpy( acontent->first, input_str,index);//存储内容
    else
        return false;

    while( index < len && input_str[index] == separator)
        ++index;

//    cout<<reject[0]<<endl;
    if( index < len ) {
        memcpy(acontent->second, input_str + index, len - index);
        return true;
    }
    return false;
}

/**
 * 注册函数
 * @param input_str　用户输入的一行登录内容
 */
void reg(char *input_str){
    //提取输入数据中的内容
    content acontent;
    if(input_str[3]== ' ' && cat_content(input_str + 4,&acontent,' ')){
        printf("%s %s\n",acontent.first,acontent.second);

    }
    else{
        printf("illgeal use, usage is: \n");
        printf("reg username password\n");
        printf("-username: be limitted in the letters without any symbol and some special like 'system' cannot be username\n");
        printf("-password: any thing you want but cannot be space only or nothing\n");
    }
}

/**
 * 登录函数
 * @param input_str 用户输入的一行内容
 */
void login(char *input_str){
    content acontent;
    if(input_str[5]== ' ' && cat_content(input_str + 6,&acontent,' ')){
        printf("%s %s\n",acontent.first,acontent.second);

    }
    else{
        printf("illgeal use, usage is: \n");
        printf("log username password\n");
        printf("-username: be limitted in the letters without any symbol and some special like 'system' cannot be username\n");
        printf("-password: any thing you want but cannot be space only or nothing\n");
    }

}

/**
 * 聊天函数
 * @param input_str 用户输入的一行聊天内容
 */
void chat(char * input_str){
    content acontent;
    if(input_str[1]!= ':' && cat_content(input_str + 1,&acontent,':')){
        printf("%s %s\n",acontent.first,acontent.second);

    }
    else{
        printf("illgeal use, usage is: \n");
        printf("@username:content\n");
        printf("-username: the goal user you want to chat with\n");
        printf("-content: words you want to send\n");
    }

}

/**
 *
 * 登出函数
 * @param input_str 用户输入的一行登出内容
 */

void logout(char *input_str){}


void choice(){
    //获取用户输入
    char input_str[BUF_LEN];
    memset(input_str, 0 ,sizeof(char) * BUF_LEN);
    cin.getline(input_str,BUF_LEN);

    if(!strncmp("reg",input_str,3)){
        reg(input_str);
    }
    else if(!strncmp("login",input_str,5)){
        login(input_str);
    }
    else if(!strncmp("logout",input_str,6)){
        reg(input_str);
    }
    else if(!strncmp("@",input_str,1)){
        chat(input_str);
    }
    else{
        printf("command not found\n");
    }
}

/*
//收到退出信号时的信号处理器
void exit_handler(int sig){
    unlink( client_pipe_name );
    unlink( client_chat_pipe );
    exit(1);
}*/



int main( int argc, char* argv[]){
    choice();

/*
    //解析服务器配置文件
    boost::property_tree::ptree root;
    boost::property_tree::read_json<boost::property_tree::ptree>("../config/serverConfig",root);
    //读取服务器文件的管道名
    const char* SERVER_REG = root.get<string>("SERVER_REG").data();
    const char* SERVER_LOG = root.get<string>("SERVER_LOG").data();
    const char* SERVER_CHAT = root.get<string>("SERVER_CHAT").data();

    //终止程序的时候 释放管道
    signal(SIGKILL, exit_handler);


    //测试服务器管道
    if( access(SERVER_REG, F_OK) + access(SERVER_LOG, F_OK) + access(SERVER_CHAT, F_OK) < 0 ){
        printf("cannot access server pipe\n");
        exit( -1 );
    }

    //打开服务器管道
    reg_fd = open(SERVER_REG ,O_WRONLY | O_NONBLOCK);
    log_fd = open(SERVER_LOG,O_WRONLY | O_NONBLOCK);
    chat_fd = open(SERVER_CHAT,O_WRONLY | O_NONBLOCK);
    if( reg_fd < 0 || log_fd < 0 || chat_fd < 0 ){
        printf("cannot open server pipe\n");
        exit(-1);
    }

    //创建自己的管道
    int pid = getpid();
    sprintf(client_pipe_name, "/tmp/client_pipe_%d", pid);
    int client_pipe = mkfifo(client_pipe_name, 0777);
    if( client_pipe < 0 ){
        printf("cannot create client pipe %s,error is %s", client_pipe_name,strerror(errno));
        exit(-1);
    }
    //open( client_pipe_name, O_WRONLY | O_RDONLY);




    while( true ){

        choice();

        if( stus ==status::Exit){
            break;
        }
        switch( stus ){
            case reg:{
                function_reg();
                break;
            }

            case log:{
                function_log();
                break;
            }
            case chat:{
                function_chat();
                break;
            }
            case help:{
                show_choice();
                break;
            }
            default: break;
        }
    }


    unlink( client_pipe_name );
    unlink( client_chat_pipe );*/
    exit(0);
}
