#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include <iostream>
using namespace std;

int main() {
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        cout << "监听套接字创建失败\n";
    }

    int opt = 10;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1602);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int ret = bind(lfd, (sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        cout << "绑定端口失败\n";
    }

    ret = listen(lfd, 50);
    if (ret == -1) {
        cout << "监听失败\n";
    }

    struct sockaddr_in client_addr;
    socklen_t client_adrr_size = sizeof(client_addr);
    int cfd = accept(lfd, (sockaddr*)&client_addr, &client_adrr_size);
    if (cfd == -1) {
        cout << "连接失败\n";
    }
    
    cout << "客户端的端口号: " << ntohs(client_addr.sin_port) << endl;

    while (1) {
        char buf[100];
        memset(buf, 0, sizeof(buf));
        int len = read(cfd, buf, sizeof(buf));
        
        if (len > 0) {
            cout << "客户端: " << buf << endl;
            write(cfd, buf, sizeof(buf));
        } else if (len == 0) {
            cout << "连接断开\n";
            break;
        } else {
            cout << "连接出现错误\n";
            break;
        }
    }
    close(cfd);
    close(lfd);
    return 0;
}