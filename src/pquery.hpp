#ifndef __PQUERY_HPP__
#define __PQUERY_HPP__

#include <cstdio>
#include <vector> 

// declaration for thread worker function
void executor(int, const std::vector<std::basic_string<char> >&);

// declaration for help in help.cpp
void show_help(void);

#ifndef PQVERSION
#define PQVERSION "1.0"
#endif

#ifndef FORK
#define FORK "MySQL"
#endif















#endif
