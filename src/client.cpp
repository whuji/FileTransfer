#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "spdlog/spdlog.h"
#include "yaml-cpp/yaml.h"


namespace fs = std::filesystem;

extern std::atomic<bool> quit;
extern YAML::Node config;


std::string handle_cmd(std::string &cmd, fs::path &path) {
	YAML::Node request;

	request["command"] = cmd;
	request["path"] = path.string();

	if (cmd == "put") {
		fs::path abs_path = fs::path(config["root_path"].as<std::string>()) / path;
		if (not fs::exists(abs_path)) {
			spdlog::critical("{} not found, cannot put", path.string());
			exit(3);
		}
		std::ifstream input(abs_path, std::ios::binary);
		size_t buflen = fs::file_size(abs_path);
		unsigned char *buf = (unsigned char*) calloc(buflen, sizeof(char));
		input.read((char*)buf, buflen);
		input.close();
		request["contentb"] = YAML::Binary(buf, buflen);
	}

	return YAML::Dump(request);
}


void handle_response(std::string cmd, fs::path path, std::string response) {
	YAML::Node res = YAML::Load(response);

	if (res["error"]) {
		spdlog::error("Error: {}", res["error"].as<std::string>());
		exit(4);
	}

	if (cmd == "list" or cmd == "put") {
		if (res["res"]){
			spdlog::info("res: {}", res["res"].as<std::string>());
		} else {
			spdlog::error("Bad reply {}", response);
		}
	} else if (cmd == "get") {
		if (not res["resb"]) {
			spdlog::error("Bad reply {}", response);
			return;
		}

		fs::path abs_path = fs::path(config["root_path"].as<std::string>()) / path;
		if (fs::exists(abs_path) and not config["enable_file_overwrite"].as<bool>()) {
			spdlog::error("{} already exists, overwriting is disabled", abs_path.string());
			return;
		}

		YAML::Binary resb = res["resb"].as<YAML::Binary>();
		std::ofstream out(abs_path, std::ios::binary);
		out.write((char*)resb.data(), resb.size());
		out.close();
		spdlog::info("{} written", abs_path.string());
	} else {
		spdlog::critical("unknown command {}", cmd);
	}
}


int client(std::string peer_ip, int peer_port, std::string &cmd, fs::path &path) {
	int ret;

	int socket_fd = socket(AF_INET,SOCK_STREAM, 0);
	struct sockaddr_in servaddr = {0};
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(peer_port);
	servaddr.sin_addr.s_addr = inet_addr(peer_ip.c_str());

	std::string request = handle_cmd(cmd, path);

	ret = connect(socket_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)); 
	if (ret != 0) {
		spdlog::error("Connect failed {}", strerror(ret));
		return -1;
	}
	spdlog::debug("Client connected");

	spdlog::debug("Sending request {}", request);
	ret = send(socket_fd, request.c_str(), request.length(), 0);
	if (ret != request.length()) {
		spdlog::error("Uncomplete send ({}/{} chars)", ret, request.length());
	}
	spdlog::debug("cmd sent");

	std::ostringstream s_res;
	char buffer[1024] = {0};
	int len;
	while((len = recv(socket_fd, buffer, sizeof(buffer), 0)) > 0) {
		  s_res.write(buffer, len);
		  bzero(buffer, len);
	}
	std::string sres = s_res.str();
	spdlog::debug("Received: {}", sres);
	handle_response(cmd, path, sres);

	spdlog::debug("Client ended");
	close(socket_fd);
	return 0;
}
