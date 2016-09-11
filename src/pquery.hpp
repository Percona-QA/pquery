#ifndef __PQUERY_HPP__
#define __PQUERY_HPP__

#include <vector>
#include <string>
#include "node.hpp"

// declaration for help functions
void print_version(void);
void show_help(void);
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
