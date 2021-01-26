#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "spdlog/spdlog.h"
#include "yaml-cpp/yaml.h"


extern YAML::Node config;


namespace fs = std::filesystem;

YAML::Node list(fs::path path) {
	YAML::Node response;

	std::string out_file = std::tmpnam(NULL);
	spdlog::debug("tmp file: {}", out_file);

	std::string bash_cmd = fmt::format("/bin/ls -l {} > {}", path.string(), out_file);
	spdlog::debug("executing {}", bash_cmd);
	int res = system(bash_cmd.c_str());

	if (res != 0) {
		response["error"] = "File not found";
	} else {
		std::ifstream output(out_file);
		output.seekg(0, output.end);
		int length = output.tellg();
		output.seekg(0, output.beg);

		char *buf = (char*) calloc(length, sizeof(char));
		output.read(buf, length);
		output.close();

		fs::remove(out_file);

		response["res"] = std::string(buf);
	}
	return response;
}


YAML::Node get(fs::path path) {
	YAML::Node response;
	if (not fs::exists(path)) {
		response["error"] = "not found";
	} else if (not fs::is_regular_file(path)) {
		response["error"] = "bad file type";
	} else {
		std::ifstream path_content(path, std::ios::binary);
		size_t buflen = fs::file_size(path);
		unsigned char *buf = (unsigned char*) calloc(buflen, sizeof(char));
		path_content.read((char*)buf, buflen);
		path_content.close();
		response["resb"] = YAML::Binary(buf, buflen);
	}
	return response;
}


YAML::Node put(fs::path path, YAML::Binary content) {
	YAML::Node response;

	if (fs::exists(path) and not config["enable_file_overwrite"].as<bool>()) {
		response["error"] = fmt::format("{} already exists, overwriting is disabled", path.string());
	} else {
		std::ofstream out(path, std::ios::binary);
		out.write((char*) content.data(), content.size());
		out.close();
		response["res"] = fmt::format("{} bytes written in {}", content.size(), path.string());
	}
	return response;
}


YAML::Node del(fs::path path) {
	YAML::Node response;
	if (not fs::exists(path)) {
		response["error"] = "not found";
	} else if (not fs::is_regular_file(path)) {
		response["error"] = "bad file type";
	} else {
		fs::remove(path);
		response["res"] = fmt::format("{} deleted", path.string());
	}
	return response;
}


std::string handle_cmd(std::string request) {
	YAML::Node req = YAML::Load(request);

	std::string cmd = req["command"].as<std::string>();

	fs::path root_path(config["root_path"].as<std::string>());
	fs::path path = root_path / req["path"].as<std::string>();

	spdlog::debug("cmd: {}, path: {}", cmd, path.string());

	YAML::Node response;
	if (cmd == std::string("list")) {
		response = list(path);
	} else if (cmd == std::string("get")) {
		response = get(path);
	} else if (cmd == std::string("put")) {
		if (not req["contentb"]) {
			response["error"] = "missing contentb";
		} else {
			response = put(path, req["contentb"].as<YAML::Binary>());
		}
	} else if (cmd == std::string("delete")) {
		response = del(path);
	} else {
		spdlog::error("Unknown command {}", cmd);
		response["error"] = "Unknown command";
	}

	YAML::Emitter rep;
	rep << response;
	return rep.c_str();
}
