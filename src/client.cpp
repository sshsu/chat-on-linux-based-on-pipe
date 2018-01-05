
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
#include <exception>
#include <pthread.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include  <iostream>
#include "myStruct.h"
#include "jsonOpera.h"
#include "infoOper.h"
using namespace std;



int reg_fd,log_fd,chat_fd;  //打开服务器管道后的对应文件描述符
status stus = OFFLINE;        //用户的初始状态
char login_username[20]={};      //登录后的各种用户信息
char user_pipe_name[50]={};
int user_pipe_fd=-1;

char client_pipe_name[50];
int client_pipe_fd;             //客户端管道的文件描述符


pthread_t thread_monitor_pipe;  //监控管道线程id
bool reg_ret ,log_ret,chat_ret,logout_ret,get_users_ret; //用于请求的等待
pthread_mutex_t reg_cond,log_cond,chat_cond,logout_cond,get_users_cond;


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
    if(input_str[3]== ' ' && cat_content(input_str + 4,&acontent,' ')){//提取输入字符串
        info ainfo;
        ainfo.serv = REG;//设置服务为注册
        memcpy(ainfo.from_name, acontent.first,sizeof(char) * strlen(acontent.first));//将名字拷贝进去
        json_add(ainfo.others,"passwd",acontent.second);//将密码以json形式放到others中去
        json_add(ainfo.others,"client_pipe",client_pipe_name);//将客户端管道名以json形式放到others中

        //发送到服务器端进行验证
        send_info(ainfo,reg_fd);

        //printf("waiting.....\n");

        //循环等待直到收到注册返回包
        pthread_mutex_lock(&reg_cond);
        reg_ret = false;
        pthread_mutex_unlock(&reg_cond);

        //阻塞到收到回复包
        bool flag = true;
        while( flag ){
            pthread_mutex_lock(&reg_cond);
            if(reg_ret == true)
                flag = false;
            pthread_mutex_unlock(&reg_cond);

        }

    }
    else{
        printf("illgeal use, usage is: \n");
        printf("reg username password\n");
        printf("-username: be limitted in the letters without any symbol and some special like 'system' cannot be username\n");
        printf("-password: any thing you want but cannot be space only or nothing\n");
    }
    //printf("//function reg finish\n");
}

/**
 * 登录函数
 * @param input_str 用户输入的一行内容
 */
void login(char *input_str){
    if( stus == ONLINE){
        printf("you have log in ,please logout\n");
        return;
    }


    content acontent;
    if(input_str[5]== ' ' && cat_content(input_str + 6,&acontent,' ')){
        info ainfo;
        ainfo.serv = LOGIN;//设置服务为登录

        memcpy(ainfo.from_name, acontent.first,sizeof(char) * strlen(acontent.first));//将名字拷贝进去
        json_add(ainfo.others,"passwd",acontent.second);//将密码以json形式放到others中去
        json_add(ainfo.others,"client_pipe",client_pipe_name);//将客户端管道名以json形式放到others中
        send_info(ainfo,log_fd); //发送到服务器端进行验证
        //阻塞等待结果
        //printf("waiting...\n");

        //循环等待直到收到登录返回包
        pthread_mutex_lock(&log_cond);
        log_ret = false;
        pthread_mutex_unlock(&log_cond);

        //阻塞到收到回复包
        bool flag = true;
        while( flag ){
            pthread_mutex_lock(&log_cond);
            if(log_ret == true)
                flag = false;
            pthread_mutex_unlock(&log_cond);
        }
    }
    else{
        printf("illgeal use, usage is: \n");
        printf("log username password\n");
        printf("-username: be limitted in the letters without any symbol and some special like 'system' cannot be username\n");
        printf("-password: any thing you want but cannot be space only or nothing\n");
    }

    //printf("//function login finish\n");
}

/**
 * 聊天函数
 * @param input_str 用户输入的一行聊天内容
 */
void chat(char * input_str){
    content acontent;
    if(stus == OFFLINE){
        printf("you haven't login ,please log in \n");
        return;
    }
    else if(strlen(input_str)==1){//如果长度为１，是向服务端索取名单
        info ainfo;
        ainfo.serv=GET_USERS;
        memcpy(ainfo.from_name,login_username,sizeof(char) * strlen(login_username));//设置发送名

        //将内容发送到服务器的聊天管道
        send_info(ainfo,chat_fd);

        //循环等待直到收到登录返回包
        pthread_mutex_lock(&get_users_cond);
        get_users_ret = false;
        pthread_mutex_unlock(&get_users_cond);

        //阻塞到收到回复包
        bool flag = true;
        while( flag ){
            pthread_mutex_lock(&get_users_cond);
            if(get_users_ret == true)
                flag = false;
            pthread_mutex_unlock(&get_users_cond);
        }

    }
    else if(input_str[1]!= ':' && cat_content(input_str + 1,&acontent,':')){
        info ainfo;
        ainfo.serv = CHAT;
        memcpy(ainfo.from_name,login_username,sizeof(char) * strlen(login_username));//设置发送名
        memcpy(ainfo.to_name, acontent.first,sizeof(char) * strlen(acontent.first));//将聊天对象名拷贝进去
        json_add(ainfo.others,"chat_content",acontent.second);//将聊天内容以json形式放到others中去

        //将内容发送到服务器的聊天管道
        send_info(ainfo,chat_fd);

    }
    else{
        printf("illgeal use, usage is: \n");
        printf("@username:content\n");
        printf("-username: the goal user you want to chat with\n");
        printf("-content: words you want to send\n");
    }

    //printf("//function chat finish\n");
}

/**
 *
 * 登出函数
 * @param input_str 用户输入的一行登出内容
 */

void logout(char *input_str){
    if(stus == OFFLINE){
        printf("you haven't log in, please log in\n");
        return ;
    }
    if(strlen(input_str) == 6){
        //printf("//send logout info to server\n");
        info ainfo;
        ainfo.serv = LOGOUT;
        memcpy(ainfo.from_name,login_username,strlen(login_username)* sizeof(char));//设置发送名
        json_add(ainfo.others,"client_pipe",client_pipe_name);//将客户端管道名以json形式放到others中
        //将退出信息发送给服务器
        send_info(ainfo,log_fd);
        //如果成功，则修改客户端的登录状态以及登录名为空

        //循环等待直到收到登录返回包
        pthread_mutex_lock(&logout_cond);
        logout_ret = false;
        pthread_mutex_unlock(&logout_cond);

        //阻塞到收到回复包
        bool flag = true;
        while( flag ){
            pthread_mutex_lock(&logout_cond);
            if(logout_ret == true)
                flag = false;
            pthread_mutex_unlock(&logout_cond);
        }

    }
    //printf("// function logout finish\n");
}


void log_out(){
    //重置登录状态
    stus = OFFLINE;
    //关闭打开的user文件
    user_pipe_fd=-1;
    close(user_pipe_fd);
    //重置用户管道名和登录名属性
    memset(user_pipe_name,0,sizeof(user_pipe_name));
    memset(login_username,0,sizeof(login_username));


}

void choice(){

    while(1) {
        //获取用户输入
        char input_str[BUF_LEN];
        memset(input_str, 0, sizeof(char) * BUF_LEN);
        cin.getline(input_str, BUF_LEN);

        if (!strncmp("reg", input_str, 3)) {
            reg(input_str);
        } else if (!strncmp("login", input_str, 5)) {
            login(input_str);
        } else if (!strcmp("logout", input_str)) {
            //printf("//in logout\n");
            logout(input_str);
        } else if (!strncmp("@", input_str, 1)) {
            chat(input_str);
        } else if(!strcmp("exit",input_str)) {
            break;
        }
        else{
            printf("command not found\n");
        }
    }
}


void deal_chat_ret(info& ret_info){
    //阻塞等待结果
    if(ret_info.serv == CHAT){//如果是一个chat 包,直接输出内容
        char chat_content[BUF_LEN];
        memset(chat_content, 0 ,sizeof(chat_content));
        json_get(chat_content, sizeof(chat_content),ret_info.others,"chat_content");
        printf("%s\n",chat_content);

    }
    else if(ret_info.ret_code == CHAT_FAILED){
        //失败的话，显示原因
        //获取返回内容
        char ret_content[BUF_LEN];
        memset(ret_content, 0 ,sizeof(ret_content));
        json_get(ret_content, sizeof(ret_content),ret_info.others,"ret_content");
        printf("message send fail and the error is:%s",ret_content);
    }
    else if(ret_info.ret_code == SEND_USERS_LIST){
        pthread_mutex_lock(&get_users_cond);
        get_users_ret =true;
        pthread_mutex_unlock(&get_users_cond);

        //printf("//get a users list info\n");
        char ret_content[BUF_LEN];
        memset(ret_content, 0 ,sizeof(ret_content));
        json_get(ret_content, sizeof(ret_content),ret_info.others,"users_list");
        printf("%s",ret_content);

    }
    //printf("//function deal_chat_ret finish\n");

}

void deal_reg_ret(info& ret_info){

    //printf("//start to parse reg return info pack\n");
    //收到了返回包
    pthread_mutex_lock(&reg_cond);
    reg_ret =true;
    pthread_mutex_unlock(&reg_cond);

    //获取返回内容
    char ret_content[BUF_LEN];
    memset(ret_content, 0 ,sizeof(ret_content));

    if( ret_info.ret_code == REG_SUCCESS){//如果注册成功了
        printf("register successfully!\n");
    }
    else{//注册失败了
        json_get(ret_content, sizeof(ret_content),ret_info.others,"ret_content");
        printf("register fail ,error is %s\n",ret_content);
    }

    //printf("//function deal_reg_ret finish\n");

}

void deal_log_ret(info& ret_info){
    if(ret_info.ret_code == LOGIN_FAILED || ret_info.ret_code == LOGIN_SUCCESS) {
        //设置状态
        pthread_mutex_lock(&log_cond);
        log_ret = true;
        pthread_mutex_unlock(&log_cond);

        //如果成功的话，设置登录状态以及登录状态名以及登录名管道
        if (ret_info.ret_code == LOGIN_SUCCESS) {
            printf("login successfully\n");
            stus = ONLINE;
            //memcpy(login_username, acontent.first, sizeof(char) * strlen(acontent.first));
            json_get(login_username, 20, ret_info.others, "login_name");
            json_get(user_pipe_name, 50, ret_info.others, "user_pipe_name");
            user_pipe_fd = open(user_pipe_name, O_RDONLY | O_NONBLOCK);
            //printf("//finish set stus to ONLINE and login username is %s and user_pipe_name is%s\n", login_username,
             //      user_pipe_name);
        } else if (ret_info.ret_code == LOGIN_FAILED) {
            //失败的话，显示原因
            //获取返回内容
            char ret_content[BUF_LEN];
            memset(ret_content, 0, sizeof(ret_content));
            json_get(ret_content, sizeof(ret_content), ret_info.others, "ret_content");
            printf("login fail and the error is: %s\n", ret_content);
        }
    }
    else{
        //设置状态
        pthread_mutex_lock(&logout_cond);
        logout_ret = true;
        pthread_mutex_unlock(&logout_cond);

        if(ret_info.ret_code == LOGOUT_SUCCESS){
            log_out();
            printf("logout successfully\n");

        }
        else if( ret_info.ret_code == LOGOUT_FAILED){
            char ret_content[BUF_LEN];
            memset(ret_content, 0, sizeof(ret_content));
            json_get(ret_content, sizeof(ret_content), ret_info.others, "ret_content");
            printf("logout fail and the error is: %s\n", ret_content);
        }

    }

    //printf("//function deal_log_ret() finish\n");
}

void* monitor_pipe(void* arg){
    //循环监控
    int client_pipe_read =0;// 从客户管道已读取的数据长度
    int user_pipe_read = 0;// 从用户管道已读取的数据长度
    int ret;
    info client_info,user_info;


    while(1){

        //读取客户端管道
        ret = read(client_pipe_fd,(void*)&client_info+client_pipe_read,sizeof(client_info) - client_pipe_read);
        if( ret != -1)
            client_pipe_read += ret;
        //已经达到一个完整的消息报,判断其中的内容进行相应的处理，将长度清０
        if( client_pipe_read == sizeof(client_info) ){
            client_pipe_read = 0;
            if( client_info.ret_code == REG_SUCCESS || client_info.ret_code == REG_FAILED ) {
                //printf("//get a reg return info pack\n");
                deal_reg_ret(client_info);
            }
            else if(client_info.ret_code == LOGIN_SUCCESS || client_info.ret_code == LOGIN_FAILED ||
                    client_info.ret_code == LOGOUT_SUCCESS || client_info.ret_code == LOGOUT_FAILED){
                //printf("//get a log return info pack\n");
                deal_log_ret(client_info);
            }

        }

        //判断是否有用户登录，如果有，则读取客户端管道中的内容
        if( stus == ONLINE ){
            ret = read(user_pipe_fd,(void*)&user_info+user_pipe_read,sizeof(user_info) - user_pipe_read);
            if( ret != -1)
                user_pipe_read += ret;
            //已经达到一个完整的消息包，判断其中的内容进行相应的处理，将长度清０
            if( user_pipe_read == sizeof(user_info) ) {
                //printf("//get a chat info pack\n");
                user_pipe_read = 0;
                deal_chat_ret(user_info);

            }

        }
    }
}


void exit_handler_oper(){
    if(user_pipe_fd > 0 )
        close(user_pipe_fd);
    close(client_pipe_fd);
    unlink( client_pipe_name );

    while( pthread_cancel( thread_monitor_pipe));//终止读取线程
    pthread_join(thread_monitor_pipe, nullptr);//回收资源

    if(stus == ONLINE)
        logout(login_username);
}

//收到退出信号时的信号处理器
void exit_handler(int sig){
    exit_handler_oper();
    exit(1);
}

void init(){

    //解析服务器配置文件
    boost::property_tree::ptree root;
    boost::property_tree::read_json<boost::property_tree::ptree>("../config/serverConfig",root);
    //读取服务器文件的管道名
    //读取服务器文件的管道名
    char SERVER_REG[BUF_LEN],SERVER_LOG[BUF_LEN],SERVER_CHAT[BUF_LEN];
    memset(SERVER_REG , 0, sizeof(SERVER_REG));
    memset(SERVER_LOG , 0, sizeof(SERVER_LOG));
    memset(SERVER_CHAT , 0, sizeof(SERVER_CHAT));
    memcpy(SERVER_REG , root.get<string>("SERVER_REG").data(), strlen(root.get<string>("SERVER_REG").data()) * sizeof(root.get<string>("SERVER_REG").data()));
    memcpy(SERVER_CHAT , root.get<string>("SERVER_CHAT").data(), strlen(root.get<string>("SERVER_CHAT").data()) * sizeof(root.get<string>("SERVER_REG").data()));
    memcpy(SERVER_LOG , root.get<string>("SERVER_LOG").data(), strlen(root.get<string>("SERVER_LOG").data()) * sizeof(root.get<string>("SERVER_REG").data()));


    //终止程序的时候 释放管道
    signal(SIGKILL, exit_handler);


    //测试服务器管道
    if( access(SERVER_REG, F_OK) + access(SERVER_LOG, F_OK) + access(SERVER_CHAT, F_OK) < 0 ){
        printf("cannot access server pipe\n");
        exit( -1 );
    }

    //打开服务器管道
    reg_fd = open(SERVER_REG ,O_WRONLY);
    log_fd = open(SERVER_LOG,O_WRONLY );
    chat_fd = open(SERVER_CHAT,O_WRONLY);
    if( reg_fd < 0 || log_fd < 0 || chat_fd < 0 ){
        printf("cannot open server pipe\n");
        exit(-1);
    }

    //创建自己的管道
    int pid = getpid();
    memset(client_pipe_name,0,sizeof(client_pipe_name));
    sprintf(client_pipe_name, "/tmp/client_pipe_%d", pid);
    int client_pipe = mkfifo(client_pipe_name, 0777);
    if( client_pipe < 0 ){
        printf("cannot create client pipe %s,error is %s", client_pipe_name,strerror(errno));
        exit(-1);
    }
    if( ( client_pipe_fd = open( client_pipe_name,O_RDONLY | O_NONBLOCK) ) < 0){
        printf("cannot open client pipe\n");
        exit(-1);
    }

}


int main( int argc, char* argv[]){

    init();

    pthread_create(&thread_monitor_pipe, NULL, monitor_pipe,NULL);
    choice();

    exit_handler_oper();
    return 0;
}
