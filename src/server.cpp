#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mutex>
#include <queue>

#include "spdlog/spdlog.h"
#include "fmt/core.h"

#include "file_operations.hpp"


extern std::atomic<bool> quit;

std::queue<int> worker_queue;

void worker(int id) {
	std::string wid(fmt::format("[worker {}]", id));
	spdlog::debug("{} ready", wid);

	while(not quit) {
		int conn = -1;
		{
			if (not worker_queue.empty()) {
				conn = worker_queue.front();
				worker_queue.pop();
			}
		}

		if (conn == -1) {
			sleep(1);
			continue;
		}

		spdlog::debug("{} conn: {}", wid, conn);

		std::ostringstream s_cmd;
		char buffer[1024] = {0};
		int len = recv(conn, buffer, sizeof(buffer), 0);
		if (len > 0) {
			do {
				s_cmd.write(buffer, len);
				bzero(buffer, len);
			} while ((len = recv(conn, buffer, sizeof(buffer), 0)) > 0);
			spdlog::debug("{} Received cmd: <{}>", wid, s_cmd.str());
			std::string response = handle_cmd(s_cmd.str());
			spdlog::debug("{} Sending response \"{}\"", wid, response);
			send(conn, response.c_str(), response.length(), 0);
			spdlog::debug("{} Response sent", wid);
		}
		close(conn);
	}
}


int server(int port) {
	int ret = 0;

	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fcntl(socket_fd, F_SETFL, O_NONBLOCK) == -1) {
		spdlog::critical("Cannot set non blocking socket");
		return -1;
	}

	struct sockaddr_in server_sockaddr;
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(port);
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int reuse = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

	ret = bind(socket_fd, (struct sockaddr*) &server_sockaddr, sizeof(server_sockaddr));
	if (ret == -1) {
		spdlog::critical("Bind failed {}", strerror(errno));
		return -1;
	}

	ret = listen(socket_fd, 20);
	if (ret == -1) {
		spdlog::critical("Listen failed {}", strerror(errno));
		return -1;
	}

	struct sockaddr_in client_addr;
	socklen_t length = sizeof(client_addr);

	while (not quit) {
		int conn = accept4(socket_fd, (struct sockaddr*) &client_addr, &length, SOCK_NONBLOCK);
		if (conn < 0) {
			if (not (errno == EWOULDBLOCK or errno == EAGAIN)) {
				spdlog::error("Accept failed {} - {}: {}", conn, errno, strerror(errno));
			}
			sleep(1);
		} else {
			spdlog::debug("Accepted ok");
			worker_queue.push(conn);
		}
	}

	spdlog::debug("Server ended");
	close(socket_fd);
	return 0;
}
