#ifndef __RANDOM_HPP__
#define __RANDOM_HPP__

#include <fstream>
#include <mysql.h>
#include <sstream>
#include <vector>
struct Table;
int run_default_load(MYSQL *conn, std::ofstream &logs,
                 std::vector<Table *> *all_tables);
int save_dictionary(std::vector<Table *> *all_tables);
void run_some_query(MYSQL *conn, std::ofstream &logs,
                    std::vector<Table *> *all_tables);
int rand_int(int upper, int lower = 0);
std::string rand_string(int upper, int lower = 0);
int execute_sql(std::string sql, MYSQL *conn);
void load_default_data(std::vector<Table *> *all_tables, MYSQL *conn);
void at_add_column(Table *table, MYSQL *conn);
void insert_data(Table *table, MYSQL *conn);
void at_drop_column(Table *table, MYSQL *conn);
void save_objects_to_file(std::vector<Table *> *all_tables);
void load_objects_from_file(std::vector<Table *> *all_tables);
void create_default_tables(std::vector<Table *> *all_tables);
void clean_up_at_end(std::vector<Table *> *all_tables);
#endif
