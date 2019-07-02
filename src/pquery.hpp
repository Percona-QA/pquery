#ifndef __PQUERY_HPP__
#define __PQUERY_HPP__

#include "common.hpp"
#include "node.hpp"
#include <string>
#include <vector>

// declaration for help functions
void print_version(void);
void show_help(void);
void show_help(Option::Opt help);
void show_help(std::string option);
void show_config_help(void);
void show_cli_help(void);

// declaration for (re)setting defaults
void
set_defaults(struct workerParams&);

void
read_section_settings(struct workerParams&, std::string, std::string);

void
create_worker(struct workerParams&);
#endif
