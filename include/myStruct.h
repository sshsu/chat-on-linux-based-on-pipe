//
// Created by woder on 1/2/18.
//

#include<string.h>

#ifndef FINAL_INFOPACK_H
#define FINAL_INFOPACK_H

#define BUF_LEN 200
//通信消息结构
class info{
public:
    char myname[20];
    char content[BUF_LEN];

    info(){
        memset(myname, 0 ,sizeof(myname));
        memset(content, 0, sizeof(content));

    }
};

//枚举状态
enum status {offline,online};

class content{
public:
    char first[BUF_LEN];
    char second[BUF_LEN];

    content(){
        memset(first,0,sizeof(first));
        memset(second, 0 ,sizeof(second));
    }
};


#endif //FINAL_INFOPACK_H
