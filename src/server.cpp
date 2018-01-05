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
#include <iostream>
#include <map>
#include <string>
#include "myStruct.h"
#include "jsonOpera.h"
#include "infoOper.h"
using namespace std;


const char* SERVER_REG = NULL;
const char* SERVER_LOG = NULL;
const char* SERVER_CHAT = NULL;
int reg_fd,log_fd, chat_fd;
int max_connect_num = 0;//最大连接数
int current_connect_num = 0;//当前连接数
pthread_t chat_id,log_id,reg_id;    //对应的线程的id


pthread_mutex_t cond;//锁头

//在线用户信息列表
std::map <std::string, userInfo> user_info_list;



void exit_handler_oper(){

    //删除创建的管道
    unlink( SERVER_REG );
    unlink( SERVER_LOG );
    unlink( SERVER_CHAT );

    //关闭注册，登录，聊天三个线程
    while(pthread_cancel( chat_id));
    while(pthread_cancel( log_id));
    while(pthread_cancel( reg_id));

    //回收对应的资源
    pthread_join( chat_id, nullptr);
    pthread_join( log_id, nullptr);
    pthread_join( reg_id, nullptr);

    printf("bye~\n");


}

void exit_handler( int sig){
    exit_handler_oper();
    exit(1);
}

/**
 *该函数主要是根据用户发来的用户名和密码，根据在线列表判断是是否有同样的用户名从而返回
 * 注册成功或者失败的数据包
 * @param ainfo　用户发来的注册数据包
 */


void reg_deal(info& ainfo){

    //搜到客户端管道位置
    char client_pipe_name[BUF_LEN];
    json_get(client_pipe_name,BUF_LEN,ainfo.others,"client_pipe");

    if( user_info_list.find(ainfo.from_name) != user_info_list.end()){//列表中找到了该用户
        printf("register name exists ,return back register infopack\n");
        //准备注册失败消息包
        info ret_fail_info;
        ret_fail_info.ret_code = REG_FAILED;                       //设置返回码为注册失败
        json_add(ret_fail_info.others,"ret_content","username exists" ); //传入错误信息

        //将数据发送回去
        send_info(ret_fail_info,client_pipe_name);
        printf("infoPack return complete\n");
    }
    else{//列表中没有找到该用户,将其插入　
        printf("register name ok ,return back register infopack\n");
        userInfo auserInfo;
        memcpy(auserInfo.user_name, ainfo.from_name,sizeof(ainfo.from_name));//填入用户名
        auserInfo.online = false;//设置登录状态为false
        json_get(auserInfo.passwd, sizeof(auserInfo.passwd),ainfo.others,"passwd");//填写当前用户的密码

        //创建用户命名管道
        char user_pipe_name[ sizeof(auserInfo.usr_pipe_name)];
        memset(user_pipe_name,0,sizeof(user_pipe_name));
        sprintf(user_pipe_name, "/tmp/usr_%s_pipe",auserInfo.user_name);

        if( access(user_pipe_name, F_OK) == -1 ){//如果不存在,则创建
            int ret = mkfifo(user_pipe_name, 0777);
            if( ret < 0 ){
                printf("cannot create client pipe %s,error is %s", user_pipe_name,strerror(errno));
            }
        }
        //填写用户管道名
        memcpy(auserInfo.usr_pipe_name,user_pipe_name,sizeof(user_pipe_name));

        //将用户数据插入在线列表中
        user_info_list[auserInfo.user_name] = auserInfo;

        //准备注册成功信息包

        info ret_success_info;
        ret_success_info.ret_code = REG_SUCCESS;                       //设置返回码为注册成功

        printf("ret_success_info.others is :%s\n",ret_success_info.others);
        //将数据发送回去
        send_info(ret_success_info,client_pipe_name);
        printf("infoPack return complete\n");
    }

}
/**
 * 读取一个数据包，然后把该数据包传送给reg_deal函数处理
 *
 */
void* reg_read(void *arg){
    unsigned int len = 0;
    info ainfo;
    while(1) {
        len += read(reg_fd, (void *) &ainfo + len, sizeof(ainfo)-len);
        if( len == sizeof(ainfo) ){
            printf("a new register reuqest,dealing now\n");
            pthread_mutex_lock(&cond);//上锁
            reg_deal(ainfo);
            pthread_mutex_unlock(&cond);
            memset(&ainfo,0 ,sizeof(ainfo));
            len = 0;
        }
    }

}
 void log_deal(info &ainfo){
     //获取客户端管道名
     char client_pipe_name[BUF_LEN];
     json_get(client_pipe_name, BUF_LEN, ainfo.others, "client_pipe");

     if(ainfo.serv == LOGIN) {


         //判断用户名是否存在
         if (user_info_list.find(ainfo.from_name) == user_info_list.end()) {
             //如果不存在，返回错误包
             printf("user does't exists,prepare infopack\n");
             info ret_fail_info;
             ret_fail_info.ret_code = LOGIN_FAILED;     //设置登录失败返回码
             json_add(ret_fail_info.others, "ret_content", "user does't exists");//设置失败内容
             send_info(ret_fail_info, client_pipe_name);//发送包
             printf("login failed infoPack has been send\n");

         } else {
             //如果存在，判断密码是否相等
             char passwd[20];
             json_get(passwd, sizeof(passwd), ainfo.others, "passwd");
             if (strcmp(passwd, user_info_list[ainfo.from_name].passwd) == 0) {
                 //如果相等，先判断最大连接数
                 if(current_connect_num < max_connect_num){
                     //如果相等,则登录，并更新用户信息列表的状态，返回登录成功信息包以及对应客户的管道名字
                     user_info_list[ainfo.from_name].online = true;
                     printf("user %s login\n", ainfo.from_name);
                     info ret_success_info;
                     ret_success_info.ret_code = LOGIN_SUCCESS;
                     json_add(ret_success_info.others, "user_pipe_name", user_info_list[ainfo.from_name].usr_pipe_name);
                     json_add(ret_success_info.others, "login_name", user_info_list[ainfo.from_name].user_name);
                     send_info(ret_success_info, client_pipe_name);
                     ++current_connect_num;
                 }
                 else{
                     printf("system has been reached max connect numer\n");
                     info ret_fail_info;
                     ret_fail_info.ret_code = LOGIN_FAILED;     //设置登录失败返回码
                     json_add(ret_fail_info.others, "ret_content", "system has been reached max connect numer, try again later");//设置失败内容
                     send_info(ret_fail_info, client_pipe_name);//发送包
                     printf("login failed infoPack has been send\n");
                 }

             } else {
                 //如果不相等，返回密码错误信息包
                 printf("password wrong,prepare infopack\n");
                 info ret_fail_info;
                 ret_fail_info.ret_code = LOGIN_FAILED;     //设置登录失败返回码
                 json_add(ret_fail_info.others, "ret_content", "password wrong");//设置失败内容
                 send_info(ret_fail_info, client_pipe_name);//发送包
                 printf("login failed infoPack has been send\n");
             }
         }
     }
     else if(ainfo.serv == LOGOUT){
         //如果是退出信息包，由于客户端完成了所有校验，所以直接退出就好
         --current_connect_num;
         user_info_list[ ainfo.from_name ].online = false;
         info ret_success_info;
         ret_success_info.ret_code = LOGOUT_SUCCESS;
         send_info(ret_success_info, client_pipe_name);
         printf("user %s logout success\n",ainfo.from_name);
     }

}

/**
 * 读取一个登录数据包
 *
 */
void* login_read(void* arg){
    unsigned int len = 0;
    info ainfo;
    while(1) {
        len += read(log_fd, (void *) &ainfo + len, sizeof(ainfo)-len);
        if( len == sizeof(ainfo) ){
            printf("a new login reuqest,dealing now\n");
            pthread_mutex_lock(&cond);//上锁
            log_deal(ainfo);
            pthread_mutex_unlock(&cond);
            memset(&ainfo,0 ,sizeof(ainfo));
            len = 0;
        }
    }

}

void chat_deal(info& ainfo){

    if(ainfo.serv == GET_USERS){
        printf("//get a get users list request\n");
        info ret_info;
        ret_info.ret_code = SEND_USERS_LIST;
        char users_lists[BUF_LEN];
        memset(users_lists,0,sizeof(users_lists));
        char buf[30];
        for(auto obj: user_info_list){//遍历整个用户列表，并且记录下来
            memset(buf, 0 ,sizeof(buf));
            sprintf(buf,"%s    %s\n",obj.second.user_name, obj.second.online? "online": "offline");
            strcat(users_lists,buf);
        }
        json_add(ret_info.others, "users_list",users_lists);
        send_info(ret_info, user_info_list[ainfo.from_name].usr_pipe_name);
        printf("users list has been send to users\n");
    }
    else if(ainfo.serv == CHAT) {
        //查看要发送的人是否存在
        if (user_info_list.find(ainfo.to_name) != user_info_list.end()) {
            //如果存在，查看是否在线
            printf("dest user exists,checking whether he is online\n");
            userInfo to_user_info = user_info_list[ainfo.to_name];
            if (to_user_info.online) {
                //如果在线,找到此人对应的管道，准备好聊天消息包进行发送
                printf("user %s is online now,sending message\n", to_user_info.user_name);
                info ret_success_info;
                ret_success_info.serv = CHAT;
                char chat_content[BUF_LEN];
                json_get(chat_content, BUF_LEN, ainfo.others, "chat_content");
                char chat_content2[BUF_LEN];
                sprintf(chat_content2, "%s:%s", ainfo.from_name, chat_content);
                json_add(ret_success_info.others, "chat_content", chat_content2);
                send_info(ret_success_info, to_user_info.usr_pipe_name);
                printf("message has been sent　to pipe:%s\n", to_user_info.usr_pipe_name);
            } else {
                //如果不在线，返回聊天失败包，通知用户对方不存在
                printf("user %s is no online now,cannot send message\n", to_user_info.user_name);
                info ret_fail_info;                     //准备聊天失败消息包
                ret_fail_info.ret_code = CHAT_FAILED;
                json_add(ret_fail_info.others, "ret_content", "dest user doesn't online now,try it later\n");
                send_info(ret_fail_info, user_info_list[ainfo.from_name].usr_pipe_name);    //发送消息包
                printf("user doesn't online info pack has been sent back\n");

            }
        } else { //如果不存在，返回聊天失败报，通知用户对方不存在
            printf("user  is no exists,cannot send message\n");
            info ret_fail_info;                     //准备聊天失败消息包
            ret_fail_info.ret_code = CHAT_FAILED;
            json_add(ret_fail_info.others, "ret_content", "dest user doesn't exists\n");
            send_info(ret_fail_info, user_info_list[ainfo.from_name].usr_pipe_name);    //发送消息包
            printf("user doesn't exists info pack has been sent back\n");
        }
    }


}

void* chat_read(void* arg){
    unsigned int len = 0;
    info ainfo;
    while(1) {
        len += read(chat_fd, (void *) &ainfo + len, sizeof(ainfo)-len);
        if( len == sizeof(ainfo) ){
            printf("a new chat reuqest,dealing now\n");
            pthread_mutex_unlock(&cond);//上锁
            chat_deal(ainfo);
            pthread_mutex_unlock(&cond);
            memset(&ainfo,0 ,sizeof(ainfo));
            len = 0;
        }
    }


}

void init(){
    //解析服务器配置文件
    boost::property_tree::ptree root;
    boost::property_tree::read_json<boost::property_tree::ptree>("/etc/serverConfig",root);
    //读取服务器文件的管道名
    char SERVER_REG[BUF_LEN],SERVER_LOG[BUF_LEN],SERVER_CHAT[BUF_LEN];
    memset(SERVER_REG , 0, sizeof(SERVER_REG));
    memset(SERVER_LOG , 0, sizeof(SERVER_LOG));
    memset(SERVER_CHAT , 0, sizeof(SERVER_CHAT));
    memcpy(SERVER_REG , root.get<string>("SERVER_REG").data(), strlen(root.get<string>("SERVER_REG").data()) * sizeof(root.get<string>("SERVER_REG").data()));
    memcpy(SERVER_CHAT , root.get<string>("SERVER_CHAT").data(), strlen(root.get<string>("SERVER_CHAT").data()) * sizeof(root.get<string>("SERVER_REG").data()));
    memcpy(SERVER_LOG , root.get<string>("SERVER_LOG").data(), strlen(root.get<string>("SERVER_LOG").data()) * sizeof(root.get<string>("SERVER_REG").data()));
    max_connect_num = atoi(root.get<string>("MAX_CONNECT_NUM").data());
    printf("//max connect num is %d\n",max_connect_num);
    printf("//get pipe name is %s %s %s\n", SERVER_REG, SERVER_LOG, SERVER_CHAT) ;

    signal(SIGKILL, exit_handler);
    signal(SIGINT, exit_handler);
    signal(SIGTERM, exit_handler);

    //check access pipe，not then create
    if( access( SERVER_REG, F_OK) == -1 ){
        if( mkfifo(SERVER_REG, 0777) == -1  ){
            printf("FIFO %s was not created\n", SERVER_REG);
            strerror(errno);
            exit( EXIT_FAILURE );
        }
        printf("create fifo %s  successfully\n", SERVER_REG);
    }

    if( access( SERVER_LOG, F_OK ) == -1 ){
        if( mkfifo( SERVER_LOG, 0777 ) != 0 ) {
            printf("FIFO %s was not created\n", SERVER_REG);
            strerror(errno);
            exit( EXIT_FAILURE );
        }
        printf("create fifo %s  successfully\n",SERVER_LOG);
    }

    if( access( SERVER_CHAT, F_OK ) == -1 ){
        if( mkfifo( SERVER_CHAT, 0777 ) != 0 ) {
            printf("FIFO %s was not created\n", SERVER_CHAT);
            strerror(errno);
            exit( EXIT_FAILURE );
        }
        printf("create fifo %s  successfully\n",SERVER_CHAT);

    }

    //open FIFO for reading
    reg_fd = open(SERVER_REG, O_RDONLY );
    log_fd = open(SERVER_LOG, O_RDONLY );
    chat_fd = open(SERVER_CHAT, O_RDONLY );

    if( reg_fd == -1 || log_fd == -1 || chat_fd == -1 ){
        printf("Could not server pipe for read only access  \n");
        printf("%s\n",strerror(errno));
        exit( EXIT_FAILURE);
    }
    else{
        printf( "open pipe file success\n");
    }
}




int main(){


    //创建守护进程
    pid_t pid;
    pid = fork();
    if( pid == -1)
        exit(-1);
    if(pid > 0 )
        exit(0);
    if(setsid() == -1)//成为新的绘画组组成和进程组组长
        exit(-1);
    chdir("/");         //更改工作目录
    int i;
    for( i = 0; i < 3; ++i) //关闭打开的fd
    {
        close(i);
        open("/dev/null", O_RDWR);
        dup(0);
        dup(0);
    }
    umask(0);


    //初始化管道等信息
    init();

    pthread_create( &reg_id, NULL,reg_read, NULL);
    pthread_create( &log_id, NULL,login_read, NULL);
    pthread_create( &chat_id, NULL,chat_read, NULL);


    //回收对应的资源
    pthread_join( chat_id, nullptr);
    pthread_join( log_id, nullptr);
    pthread_join( reg_id, nullptr);
    return 0;
}

