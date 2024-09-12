#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#define nums 128

typedef struct SocketInfo {
    int fd;
    int epfd;
}SocketInfo;

void* acceptConn(void* arg) {
    printf("pthread ID:%d\n", pthread_self());
    SocketInfo* info = (SocketInfo*)arg;
    // 建立新的连接
    int cfd = accept(info->fd,NULL,NULL);
    // 将文件描述符设置为非阻塞
    // 得到文件描述符的属性
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);
    
    
    // 新得到的文件描述符添加到epoll模型中, 下一轮循环的时候就可以被检测了
    // 通信的文件描述符检测读缓冲区数据的时候设置为边沿模式
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;    // 读缓冲区是否有数据
    ev.data.fd = cfd;
    int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);   
    if (ret == -1)
    {
        perror("epoll_ctl-accept");
        free(info);
        exit(0);
    }

    free(info);
    return NULL;
}

void* communication(void* arg) {
    SocketInfo* info = (SocketInfo*)arg;
    printf("pthread ID:%d\n",pthread_self());
    // 处理通信的文件描述符
    // 接收数据
    char readbuf[1024];

    memset(readbuf, 0, sizeof(readbuf));
    // 循环读数据
    while (1)
    {
       
        int len = recv(info->fd, readbuf, sizeof(readbuf), 0);
        if (len == 0)
        {
            
            printf("客户端断开了连接...\n");
            // 将这个文件描述符从epoll模型中删除
            epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
            close(info->fd);
            break;
        }
        else if (len > 0)
        {
            // 通信
            // 接收的数据打印到终端
            write(STDOUT_FILENO, readbuf, len);
            send(info->fd, readbuf, len, 0);
            
        }
        else
        {
            // len == -1
            // 判断对方是否读取到末尾，还是发生了异常
            // errno 是#include <errno.h>全局变量，
            if (errno == EAGAIN)  //读取到文件末尾
            {
                printf("data had readed...\n");
                            
                break;
            }
            else
            {
                perror("recv");
                break;
            }
        }
    }
    free(info);
    return NULL;
}


int main()
{

    // 1.创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);

    if (lfd == -1) {
        perror("socket");
        exit(0);
    }

    //2.将监听套接字与本地ip、端口绑定在一起
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(30001);   //转成大端端口
    /*
       INADDR_ANY代表本机的所有IP, 假设有三个网卡就有三个IP地址
       这个宏可以代表任意一个IP地址
       这个宏一般用于本地的绑定操作
    */
    addr.sin_addr.s_addr = INADDR_ANY;   // 这个宏的值为0 == 0.0.0.0
    //绑定
    int ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1) {
        perror("bind");
        exit(0);
    }

    //3.设置监听
    int lit = listen(lfd, 128);  //最多128个
    if (lit == -1) {
        perror("listen");
        exit(0);
    }

    //创建epoll实例，参数大于0就可以
    int epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        exit(0);
    }

    //为epoll实例添加监听套接字节点
    struct epoll_event ev;
    ev.events = EPOLLIN;  // 检测lfd读读缓冲区是否有数据
    ev.data.fd = lfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);   //ev在函数里会进行一次值的拷贝


    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(evs[0]);
    //循环检测
    while (1)
    {
        // 调用一次, 检测一次
        int num = epoll_wait(epfd, evs, size, -1);     //会将检测到的写入evs,从0到num-1的位置上
        for (int i = 0; i < num; ++i)
        {
            int fd = evs[i].data.fd;
            SocketInfo* info = (SocketInfo*)malloc(sizeof(SocketInfo));
            info->fd = fd;
            info->epfd = epfd;
            if (fd == lfd) {

                //***epoll函数都是线程安全的，因此无需加锁***
                
                //如果是监听文件描述符(监听套接字)
                //创建子线程接收新连接
                pthread_t tid;
                pthread_create(&tid,NULL,acceptConn,info);
                pthread_detach(tid);
            }
            else {
                //创建子线程通信
                pthread_t tid;
                pthread_create(&tid, NULL, communication, info);
                pthread_detach(tid);
            }
        }

    }
    return 0;
}

