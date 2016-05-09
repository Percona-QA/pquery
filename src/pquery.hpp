#ifndef __PQUERY_HPP__
#define __PQUERY_HPP__

#include <vector>
#include <string>

// declaration for help functions
void print_version(void);
void show_help(void);
void show_config_help(void);
void show_cli_help(void);

#ifndef PQVERSION
#define PQVERSION "2.0"
#endif

#ifndef FORK
#define FORK "MySQL"
#endif
#endif
