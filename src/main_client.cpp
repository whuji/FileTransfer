#include <iostream>
#include <sstream>
#include <thread>
#include <filesystem>
#include <unistd.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "yaml-cpp/yaml.h"

#include "main_common.hpp"
#include "client.hpp"


namespace fs = std::filesystem;

const char* parse_args(int argc, char *argv[]) {
	int ind;
	char opt[] = "dc:";
	const char *config_file = NULL; 

	while ((ind = getopt(argc, argv, opt)) != -1) {
		switch (ind) {
			case 'd':
				spdlog::set_level(spdlog::level::debug);
				break;
			case 'c':
				config_file = optarg;
				break;
			default:
				return NULL;
		}
	}

	return config_file;
}


int main(int argc, char *argv[]) {
	install_signal();

	auto logger = spdlog::stdout_color_mt("ft_client");
	spdlog::set_default_logger(logger);
	spdlog::info("Started");

	const char* config_file = parse_args(argc, argv);

	if (config_file == NULL) {
		spdlog::critical("No config file specified");
		exit(1);
	}

	if (optind == argc) {
		spdlog::critical("No command specified");
		exit(2);
	}

	if (optind + 1 == argc) {
		spdlog::critical("No path specified");
		exit(2);
	}

	load_config_file(config_file);

	std::string cmd = argv[optind];
	fs::path path = fs::path(argv[optind +1]);

	spdlog::debug("config: \n{}", YAML::Dump(config));

	client(config["peer_address"].as<std::string>(), config["peer_port"].as<int>(), cmd, path);

	return 0;
}
