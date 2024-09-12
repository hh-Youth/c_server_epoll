//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <string.h>
//#include <arpa/inet.h>
//#include <pthread.h>
//#include<sys/epoll.h>
//
//#define nums 128
//
//int main()
//{
//   
//    // 1.创建监听的套接字
//    int lfd = socket(AF_INET, SOCK_STREAM, 0);
//
//    if (lfd == -1) {
//        perror("socket");
//        exit(0);
//    }
//
//    //2.将监听套接字与本地ip、端口绑定在一起
//    struct sockaddr_in addr;
//    addr.sin_family = AF_INET;
//    addr.sin_port = htons(30001);   //转成大端端口
//    /*
//       INADDR_ANY代表本机的所有IP, 假设有三个网卡就有三个IP地址
//       这个宏可以代表任意一个IP地址
//       这个宏一般用于本地的绑定操作
//    */
//    addr.sin_addr.s_addr = INADDR_ANY;   // 这个宏的值为0 == 0.0.0.0
//    //绑定
//    int ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
//    if (ret == -1) {
//        perror("bind");
//        exit(0);
//    }
//
//    //3.设置监听
//    int lit = listen(lfd, 128);  //最多128个
//    if (lit == -1) {
//        perror("listen");
//        exit(0);
//    }
//
//    //创建epoll实例，参数大于0就可以
//    int epfd = epoll_create(1);
//    if (epfd==-1) {
//        perror("epoll_create");
//        exit(0);
//    }
//
//    //为epoll实例添加监听套接字节点
//    struct epoll_event ev;
//    ev.events = EPOLLIN;  // 检测lfd读读缓冲区是否有数据
//    ev.data.fd = lfd;
//    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, & ev);   //ev在函数里会进行一次值的拷贝
//    
//
//    struct epoll_event evs[1024];
//    int size = sizeof(evs) / sizeof(evs[0]);
//    //循环检测
//    while (1)
//    {
//        // 调用一次, 检测一次
//        int num = epoll_wait(epfd, evs, size, -1);     //会将检测到的写入evs,从0到num-1的位置上
//        for (int i = 0; i < num; ++i)
//        {
//            int fd = evs[i].data.fd;
//            
//            if (fd==lfd) {
//                //如果是监听文件描述符(监听套接字)
//                int tfd = accept(fd,NULL,NULL);
//                //新得到的文件描述符添加到epoll模型中, 下一轮循环的时候就可以被检测了
//
//                //重复利用ev
//                ev.events = EPOLLIN;
//                ev.data.fd = tfd;
//                int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev);
//                if (ret==-1) {
//                    perror("epoll_ctl-accept");
//                    exit(0);
//                } 
//            }
//            else {
//                char buf[1024];
//                memset(buf, 0, sizeof(buf));
//                int len = recv(fd,buf,sizeof(buf),0);
//                if (len==0) {
//                    //客户端关闭连接
//                    printf("客户端已经断开了连接\n");
//                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
//                    close(fd);
//                }
//                else if (len > 0) {
//                    printf("客户端say:%s\n",buf);
//                    //发送给客户端
//                    send(fd,buf,len,0);
//                }
//                else {
//                    perror("recv");
//                    exit(0);
//                }
//
//            }
//        }
//
//    }
//    return 0;
//}
//


/*

    当在服务器端循环调用epoll_wait()的时候，就会得到一个就绪列表，并通过该函数的第二个参数传出：
    struct epoll_event evs[1024];
    int num = epoll_wait(epfd, evs, size, -1);

*/



