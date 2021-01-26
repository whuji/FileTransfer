#include <atomic>
#include <signal.h>
#include <unistd.h>

#include "yaml-cpp/yaml.h"
#include "spdlog/spdlog.h"


std::atomic<bool> quit(false);
YAML::Node config;


void load_config_file(const char* config_file) {
	try {
		config = YAML::LoadFile(config_file);
	} catch(const YAML::BadFile &e) {
		spdlog::critical("Configuration file not found ({})", e.what());
		exit(1);
	} catch(const YAML::ParserException &e) {
		spdlog::critical("Bad configuration file format ({})", e.what());
		exit(1);
	} catch(const std::exception &e) {
		spdlog::critical("Invalid configuration file ({})", e.what());
		exit(1);
	}
}


void signal_handler(int) {
	spdlog::debug("Signal handler triggered");
	quit = true;
}


void install_signal(void) {
	struct sigaction struct_sigaction = {0};
	sigset_t sig_block;
	struct_sigaction.sa_handler = signal_handler;
	sigemptyset(&struct_sigaction.sa_mask);
	sigaddset(&struct_sigaction.sa_mask, SIGINT);
	sigaddset(&struct_sigaction.sa_mask, SIGTERM);
	sigaction(SIGINT, &struct_sigaction, nullptr);
	sigaction(SIGTERM, &struct_sigaction, nullptr);
	sigemptyset(&sig_block);
	sigaddset(&sig_block, SIGINT);
	sigaddset(&sig_block, SIGTERM);
	sigprocmask(SIG_UNBLOCK, &sig_block, nullptr);
}
