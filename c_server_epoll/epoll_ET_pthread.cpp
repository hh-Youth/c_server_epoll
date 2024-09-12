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
    // �����µ�����
    int cfd = accept(info->fd,NULL,NULL);
    // ���ļ�����������Ϊ������
    // �õ��ļ�������������
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);
    
    
    // �µõ����ļ���������ӵ�epollģ����, ��һ��ѭ����ʱ��Ϳ��Ա������
    // ͨ�ŵ��ļ��������������������ݵ�ʱ������Ϊ����ģʽ
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;    // ���������Ƿ�������
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
    // ����ͨ�ŵ��ļ�������
    // ��������
    char readbuf[1024];

    memset(readbuf, 0, sizeof(readbuf));
    // ѭ��������
    while (1)
    {
       
        int len = recv(info->fd, readbuf, sizeof(readbuf), 0);
        if (len == 0)
        {
            
            printf("�ͻ��˶Ͽ�������...\n");
            // ������ļ���������epollģ����ɾ��
            epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
            close(info->fd);
            break;
        }
        else if (len > 0)
        {
            // ͨ��
            // ���յ����ݴ�ӡ���ն�
            write(STDOUT_FILENO, readbuf, len);
            send(info->fd, readbuf, len, 0);
            
        }
        else
        {
            // len == -1
            // �ж϶Է��Ƿ��ȡ��ĩβ�����Ƿ������쳣
            // errno ��#include <errno.h>ȫ�ֱ�����
            if (errno == EAGAIN)  //��ȡ���ļ�ĩβ
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

    // 1.�����������׽���
    int lfd = socket(AF_INET, SOCK_STREAM, 0);

    if (lfd == -1) {
        perror("socket");
        exit(0);
    }

    //2.�������׽����뱾��ip���˿ڰ���һ��
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(30001);   //ת�ɴ�˶˿�
    /*
       INADDR_ANY������������IP, ����������������������IP��ַ
       �������Դ�������һ��IP��ַ
       �����һ�����ڱ��صİ󶨲���
    */
    addr.sin_addr.s_addr = INADDR_ANY;   // ������ֵΪ0 == 0.0.0.0
    //��
    int ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1) {
        perror("bind");
        exit(0);
    }

    //3.���ü���
    int lit = listen(lfd, 128);  //���128��
    if (lit == -1) {
        perror("listen");
        exit(0);
    }

    //����epollʵ������������0�Ϳ���
    int epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        exit(0);
    }

    //Ϊepollʵ����Ӽ����׽��ֽڵ�
    struct epoll_event ev;
    ev.events = EPOLLIN;  // ���lfd�����������Ƿ�������
    ev.data.fd = lfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);   //ev�ں���������һ��ֵ�Ŀ���


    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(evs[0]);
    //ѭ�����
    while (1)
    {
        // ����һ��, ���һ��
        int num = epoll_wait(epfd, evs, size, -1);     //�Ὣ��⵽��д��evs,��0��num-1��λ����
        for (int i = 0; i < num; ++i)
        {
            int fd = evs[i].data.fd;
            SocketInfo* info = (SocketInfo*)malloc(sizeof(SocketInfo));
            info->fd = fd;
            info->epfd = epfd;
            if (fd == lfd) {

                //***epoll���������̰߳�ȫ�ģ�����������***
                
                //����Ǽ����ļ�������(�����׽���)
                //�������߳̽���������
                pthread_t tid;
                pthread_create(&tid,NULL,acceptConn,info);
                pthread_detach(tid);
            }
            else {
                //�������߳�ͨ��
                pthread_t tid;
                pthread_create(&tid, NULL, communication, info);
                pthread_detach(tid);
            }
        }

    }
    return 0;
}

