#ifndef __RANDOM_HPP__
#define __RANDOM_HPP__

#include "common.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <document.h>
#include <filereadstream.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <mysql.h>
#include <prettywriter.h>
#include <random>
#include <sstream>
#include <string.h>
#include <vector>
#include <writer.h>

struct Table;
struct Thd1;
int run_default_load(Thd1 *thd);
void run_some_query(Thd1 *thd);

int save_dictionary(std::vector<Table *> *all_tables);
int rand_int(int upper, int lower = 0);
std::string rand_string(int upper, int lower = 0);
int execute_sql(std::string sql, Thd1 *thd);
void load_default_data(std::vector<Table *> *all_tables, Thd1 *thd);
void insert_data(Table *table, MYSQL *conn);
void save_objects_to_file(std::vector<Table *> *all_tables);
void load_objects_from_file(std::vector<Table *> *all_tables);
void create_default_tables(std::vector<Table *> *all_tables);
void clean_up_at_end(std::vector<Table *> *all_tables);
#endif
