#include<iostream>
#include<string>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<map>

using namespace std;

//最大连接数
const int MAX_CONN = 1024;

//用于保存客户端信息
struct Client
{
	int sockfd;
	string name;
};

void sys_error(const char *error) {
	perror(error);
	exit(EXIT_FAILURE);
}

int main() {
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len;
	char client_ip[INET_ADDRSTRLEN];

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(9999);

	struct epoll_event evs[MAX_CONN];

	//创建epoll实例
	int epfd = epoll_create1(0);
	if (epfd < 0) sys_error("epoll create1 error\n");

	//创建监听socket
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) sys_error("socket create error\n");
	
	int opt = 1; //设置端口复用
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));

	//绑定本地ip和端口
	int ret = bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (ret < 0) sys_error("bind error\n");

	//设置最大连接数
	ret = listen(listen_fd, 1024);
	if (ret < 0) sys_error("listen error\n");

	//将监听的socket加入epoll 
	struct epoll_event listen_ev;
	listen_ev.events = EPOLLIN;
	listen_ev.data.fd = listen_fd;
	 
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &listen_ev);
	if (ret < 0) sys_error("epoll_ctl error\n");

	//保存客户端信息
	map<int, Client> clients;

	//循环监听
	while (1) {
		int cnt = epoll_wait(epfd, evs, MAX_CONN, -1); //-1表示阻塞监听
		if (cnt < 0) sys_error("epoll_wait error\n");

		for (int i = 0; i < cnt; i++) {
			int fd = evs[i].data.fd;
			//如果是监听fd的事件，说明有客户端请求连接
			if (fd == listen_fd) {
				int client_sockfd = accept(listen_fd, (struct sockaddr*) & client_addr, &client_addr_len);
				if (client_sockfd < 0) sys_error("accept error\n");

				//将客户端的socket加入epoll
				struct epoll_event client_ev;
				client_ev.events = EPOLLIN;
				client_ev.data.fd = client_sockfd;

				ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client_sockfd, &client_ev);
				if (ret < 0) sys_error("epoll_ctl error\n");

				inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
				printf("%s正在连接...\n", client_ip);

				//保存该客户端的信息
				Client client;
				client.sockfd = client_sockfd;
				client.name = "";
				clients[client_sockfd] = client;
			}
			else { //处理客户端的消息
				char buffer[1024] = {0}; //初始化缓冲区
				ssize_t n = read(fd, buffer, sizeof(buffer) - 1); //保留一个字节用于 '\0'
				if (n < 0) {
					// 检查错误类型
					if (errno == ECONNRESET || errno == EPIPE) {
						printf("Client %d disconnected unexpectedly.\n", fd);
						close(fd);
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
						clients.erase(fd);
					}
					else {
						perror("read error");
					}
				}
				else if (n == 0) { //表示该客户端断开连接
					printf("Client %d disconnected.\n", fd);
					close(fd);

					ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					if (ret < 0) sys_error("epoll_ctl error\n");

					clients.erase(fd);

					continue;
				}
				else {
					buffer[n] = '\0'; //确保字符串以 '\0' 结尾
					string msg(buffer, n); //把字符串数组转换为string
					//如果该客户端name为空，说明该消息是第一条消息，即用户名
					if (clients[fd].name == "") {
						clients[fd].name = msg;
					}
					else { //否则是聊天消息，把该消息发给除它之外的所有客户端
						string full_msg = '[' + clients[fd].name + "]: " + msg;
						for (auto& c : clients) {
							if (c.first != fd) { //除去自己
								write(c.first, full_msg.c_str(), full_msg.size());
							}
						}
					}
				}
			}
		}
	}

	//释放套接字和epoll实例
	close(epfd);
	close(listen_fd);
	 
	return 0;
}