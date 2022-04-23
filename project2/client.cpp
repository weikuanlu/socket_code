#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include <iostream>
using namespace std;

int main() {
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1) {
        cout << "套接字创建失败\n";
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8989);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int ret = connect(cfd, (sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        cout << "连接失败\n";
    }

    while (1) {
        char buf[100] = "Hello World!";
        write(cfd, buf, sizeof(buf));

        memset(buf, 0, sizeof(buf));
        int len = read(cfd, buf, sizeof(buf));
        if (len > 0) {
            cout << "服务器回传: " << buf << endl;
        } else {
            cout << "连接出现错误\n";
            break;
        }
        sleep(1);
    }
    close(cfd);
    return 0;
}