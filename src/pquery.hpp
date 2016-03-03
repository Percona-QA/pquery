#ifndef __PQUERY_HPP__
#define __PQUERY_HPP__

#include <vector>
#include <string>

// declaration for thread worker function
void executor(int, const std::vector<std::basic_string<char> >&);

// declaration for initial connection
void try_connect();

// declaration for help function
void show_help(void);

#ifndef PQVERSION
#define PQVERSION "2.0"
#endif

#ifndef FORK
#define FORK "MySQL"
#endif

#endif
