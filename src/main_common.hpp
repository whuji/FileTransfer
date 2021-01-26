#pragma once

#include "yaml-cpp/yaml.h"

extern YAML::Node config;

void load_config_file(const char*);
void signal_handler(int);
void install_signal(void);
