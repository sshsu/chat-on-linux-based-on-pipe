//
// Created by woder on 1/2/18.
//

#include<string.h>
#include<string>

#ifndef FINAL_INFOPACK_H
#define FINAL_INFOPACK_H

#define BUF_LEN 200

enum service {SER_DEF,REG,LOGIN,CHAT,GET_USERS,LOGOUT};
enum ret{RET_DEF,REG_FAILED,REG_SUCCESS,LOGIN_FAILED,LOGIN_SUCCESS,CHAT_FAILED,CHAT_SUCCESS,LOGOUT_FAILED,LOGOUT_SUCCESS,SEND_USERS_LIST};
//通信消息结构
class info{
public:
    char from_name[20];
    char to_name[20];
    char others[BUF_LEN];//一个以json形式存储其余信息的string
    service serv;
    ret ret_code;
    info(){
        memset(from_name, 0 ,sizeof(from_name));
        memset(to_name, 0, sizeof(to_name));
        memset(others,0,sizeof(others));
        memcpy(others,"{}",sizeof(char)*strlen("{}"));
        serv = SER_DEF;
        ret_code = RET_DEF;
    }
};

//枚举状态
enum status {OFFLINE,ONLINE};

class content{
public:
    char first[BUF_LEN];
    char second[BUF_LEN];

    content(){
        memset(first,0,sizeof(first));
        memset(second, 0 ,sizeof(second));
    }
};

//用户信息结构

class userInfo{
public:
    char user_name[20];
    char passwd[20];
    bool online;
    char usr_pipe_name[50];
    userInfo(){
        memset(user_name,0 ,sizeof(user_name));
        memset(passwd,0, sizeof(passwd));
        online = false;
        memset(usr_pipe_name, 0 , sizeof(usr_pipe_name));
    }
};

#endif //FINAL_INFOPACK_H
