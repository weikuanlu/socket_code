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
#include <pthread.h>

#include <iostream>
using namespace std;

struct socketinfo {
    int fd;
    int epfd;
};

void *Accept(void *args) {
    socketinfo* info = (socketinfo*)args;
    // 建立新的连接
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    int cfd = accept(info->fd, (struct sockaddr*)&client_addr, &client_addr_size);
    char ip[24] = {0};
    cout << "线程ID: " << pthread_self();
    cout << " 客户端的IP地址: " << inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip, sizeof(ip));
    cout << " 端口: " << ntohs(client_addr.sin_port) << endl;

    // 新得到的文件描述符添加到epoll模型中, 下一轮循环的时候就可以被检测了
    struct epoll_event ev;
    ev.events = EPOLLIN;    // 读缓冲区是否有数据
    ev.data.fd = cfd;
    int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
    if(ret == -1)
    {
        perror("epoll_ctl-accept");
        exit(0);
    }
    delete info;
    return NULL;
}

void *communication(void *args) {
    socketinfo* info = (socketinfo*)args;

    // 接收数据
    char buf[50];
    memset(buf, 0, sizeof(buf));
    int len = read(info->fd, buf, sizeof(buf));
    if(len == 0) {
        cout << "客户端已经断开了连接\n";
        // 将这个文件描述符从epoll模型中删除
        epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
        close(info->fd);
    } else if(len > 0) {
        cout << "线程ID: " << pthread_self() << " 客户端say: " << buf << endl;
        write(info->fd, buf, len);
    } else {
        perror("recv");
        exit(0);
    }
    delete info;
    return NULL;
}

int main(int argc, const char* argv[])
{
    // 创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("socket error");
        exit(1);
    }

    // 绑定
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(10002);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 绑定端口
    int ret = bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (ret == -1) {
        perror("bind error");
        exit(1);
    }

    // 监听
    ret = listen(lfd, 64);
    if (ret == -1) {
        perror("listen error");
        exit(1);
    }

    // 现在只有监听的文件描述符
    // 所有的文件描述符对应读写缓冲区状态都是委托内核进行检测的epoll
    // 创建一个epoll模型
    int epfd = epoll_create(100);
    if (epfd == -1) {
        perror("epoll_create");
        exit(0);
    }

    // 往epoll实例中添加需要检测的节点, 现在只有监听的文件描述符
    struct epoll_event ev;
    ev.events = EPOLLIN;    // 检测lfd读读缓冲区是否有数据
    ev.data.fd = lfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if (ret == -1) {
        perror("epoll_ctl");
        exit(0);
    }

    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(struct epoll_event);
    // 持续检测
    while(1)
    {
        // 调用一次, 检测一次
        int num = epoll_wait(epfd, evs, size, -1);
        for (int i=0; i < num; ++i)
        {
            // 取出当前的文件描述符
            int curfd = evs[i].data.fd;
            struct socketinfo *info = new socketinfo;
            info->fd = curfd;
            info->epfd = epfd;
            pthread_t tid;
            // 判断这个文件描述符是不是用于监听的
            if (curfd == lfd) {
                pthread_create(&tid, NULL, Accept, info);
                pthread_detach(tid);
            }
            else {
                pthread_create(&tid, NULL, communication, info);
                pthread_detach(tid);
            }
        }
    }

    return 0;
}