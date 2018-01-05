## 文件说明
```
├── bin
│   ├── wangyishen_client 客户端
│   └── wangyishen_server　服务端
├── cmake-build-debug
├── CMakeLists.txt
├── config
│   └── serverConfig
├── include				头文件
│   ├── infoOper.h
│   ├── jsonOpera.h
│   └── myStruct.h
├── makescripts
├── README				
└── src 				cpp文件
    ├── client.cpp
    ├── infoOper.cpp
    ├── jsonOpera.cpp
    └── server.cpp
```
项目结构如上图所述，其中bin下的两个文件分别为客户端和服务端,config中存放这服务器的配置文件serverConfig，include存放着头文件,src中存放代码文件,makescripts存放这编译文件


## 如何运行
获取整个工程后在shell中使用以下命令编译程序后就可以发现bin下有对应的文件，也可以直接在终端中执行makescripts的语句进行编译

```
sudo bash makescripts
```

在当前目录中使用以下命令，将服务器的配置文件拷贝到/etc文件夹下

```
sudo cp /config/serverConfig /etc/
```
使用以下命令运行服务端代码

```
sudo ./bin/wangyishen_server
```

使用以下命令运行客户端代码

```
sudo ./bin/wangyishen_client
```

## 服务端配置文件

SERVER_REG，SERVER_LOG，SERVER_CHAT字段代表是服务器要打开的三个管道，其对应值的是管道名，如果要修改，请确保同时修改config/serverConfig，否则程序将无法运行

MAX_CONNECT_NUM　代表的是服务器的最大连接数量，想要扩大或缩小，修改对应字段的数字即可


## 客户端有哪些功能

	1### 注册功能

```
reg username passwd
```

	1### 登录功能

```
login username passwd
```

	1### 登出功能
```
logout
```

	1### 聊天功能
```
@user:words
```

	1### 查询所有用户状态功能

```
@
```