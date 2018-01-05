

#cmake版本
cmake_minimum_required(VERSION 3.8)

#项目名字
project(final)

#头文件目录
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR}/include)
#INCLUDE_DIRECTORIES(/usr/include)
#源文件目录， 将当前目录中的所有源文件名称赋值给变量 DIR_SRCS
AUX_SOURCE_DIRECTORY(./src DIR_SRCS)

#设置环境变量
#设置变异标准 c++11
set(CMAKE_CXX_STANDARD 11)
#将WEB_SERVER环境变量设置成${DIR_SRCS}
set(final ${DIR_SRCS} src/client.cpp include/myStruct.h include/jsonOpera.h src/jsonOpera.cpp src/server.cpp src/infoOper.cpp include/infoOper.h)

#将需要编译的所有可执行文件${WBB_SERVER}，编译成文件${PROJECT_NAME}
ADD_EXECUTABLE(${PROJECT_NAME} ${final} include/jsonOpera.h src/jsonOpera.cpp src/server.cpp src/infoOper.cpp include/infoOper.h)
#ADD_EXECUTABLE(${PROJECT_NAME} src/main.cpp src/http_conn.cpp src/threadpool.cpp} )
#添加可执行文件要用到的库
TARGET_LINK_LIBRARIES(${PROJECT_NAME} m)
