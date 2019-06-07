
#ifndef __RANDOM_HPP__
#define __RANDOM_HPP__
#include "node.hpp"
int run_som_load(MYSQL *conn, std::ofstream &logs);
int save_dictionary();
void run_some_query(MYSQL *conn, std::ofstream &logs);
struct Table;
#endif
