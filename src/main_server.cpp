#include <iostream>
#include <thread>
#include <unistd.h>
#include <atomic>
#include <signal.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "yaml-cpp/yaml.h"

#include "main_common.hpp"
#include "server.hpp"


const char* parse_args(int argc, char *argv[]) {
	int ind;
	char opt[] = "dc:";
	const char *config_file = NULL; 

	while((ind = getopt(argc, argv, opt)) != -1) {
		switch(ind){
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

	auto logger = spdlog::stdout_color_mt("ft_server");
	spdlog::set_default_logger(logger);
	spdlog::info("Started");

	const char* config_file = parse_args(argc, argv);

	if (config_file == NULL) {
		spdlog::critical("No config file specified");
		exit(1);
	}

	load_config_file(config_file);

	std::cout << config << std::endl;

	std::thread worker1(worker, 1);
	std::thread worker2(worker, 2);
	std::thread worker3(worker, 3);
	std::thread worker4(worker, 4);

	server(config["listen_port"].as<int>());

	worker1.join();
	worker2.join();
	worker3.join();
	worker4.join();

	return 0;
}
