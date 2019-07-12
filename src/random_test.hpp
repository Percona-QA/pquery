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
#include <atomic>
#define INNODB_16K_PAGE_SIZE 16
#define INNODB_8K_PAGE_SIZE 8
#define INNODB_32K_PAGE_SIZE 32
#define INNODB_64K_PAGE_SIZE 64
#define MIN_SEED_SIZE 10000
#define MAX_SEED_SIZE 100000
#define MAX_RANDOM_STRING_SIZE 32

#define opt_int(a) options->at(Option::a)->getInt();
#define opt_int_set(a, b) options->at(Option::a)->setInt(b);
#define opt_bool(a) options->at(Option::a)->getBool();
#define opt_string(a) options->at(Option::a)->getString();

/* Different table type supported by tool */
enum TABLE_TYPES { PARTITION, NORMAL, TEMPORARY, TABLE_MAX };
/* Column Basic Properties */


int rand_int(int upper, int lower = 0);

struct Table;

struct Column {
  Column(std::string name, Table *table);
  Column(std::string name, std::string type, bool is_null, int len,
         Table *table);
  Column(std::string name, Table *table, int type);
  Column(const Column &column);
  std::string rand_value();
  template <typename Writer> void Serialize(Writer &writer) const;
  std::string name_;
  int type_;
  bool null = false;
  int length = 0;
  std::string default_value;
  bool primary_key = false;
  bool generated = false;
  Table *table_;
};

struct Ind_col {
  Ind_col(Column *c);
  Ind_col(Column *c, bool d);
  template <typename Writer> void Serialize(Writer &writer) const;

  Column *column;
  bool desc = false;
  int length = 0;
};

struct Index {
  Index(std::string n);
  void AddInternalColumn(Ind_col *column);
  template <typename Writer> void Serialize(Writer &writer) const;
  ~Index();

  std::string name_;
  std::vector<Ind_col *> *columns_;
};

struct Thd1 {
  Thd1(int id, std::ofstream &tl, MYSQL *c)
      : thread_id(id), thread_log(tl), conn(c){};
  int thread_id;
  std::ofstream &thread_log;
  MYSQL *conn;
  int seed;
};


/* Table basic properties */
struct Table {
public:
  TABLE_TYPES type;
  Table(std::string n);
  Table(std::string n, int max_pk);
  static Table *table_id(TABLE_TYPES choice, int id);
  std::string Table_defination();
  /* methods to create table of choice */
  void AddInternalColumn(Column *column) { columns_->push_back(column); }
  void AddInternalIndex(Index *index) { indexes_->push_back(index); }
  void CreateDefaultColumn();
  void CreateDefaultIndex();
  void CopyDefaultColumn(Table *table);
  void CopyDefaultIndex(Table *table);
  void DropCreate(Thd1 *thd);
  void Optimize(Thd1 *thd);
  void Analyze(Thd1 *thd);
  void Truncate(Thd1 *thd);
  void SetEncryption(Thd1 *thd);
  void InsertRandomRow(Thd1 *thd, bool islock);
  void DropColumn(Thd1 *thd);
  void AddColumn(Thd1 *thd);
  void DeleteRandomRow(Thd1 *thd);
  void UpdateRandomROW(Thd1 *thd);
  void SelectRandomRow(Thd1 *thd);
  void SelectAllRow(Thd1 *thd);
  void DeleteAllRows(Thd1 *thd);
  void ColumnRename(Thd1 *thd);
  /* end */
  template <typename Writer> void Serialize(Writer &writer) const;
  virtual ~Table();

  std::string name_;
  bool encryption = false;
  int key_block_size = 0;
  std::string data_directory;
  std::string row_format;
  std::string tablespace;
  std::vector<Column *> *columns_;
  std::vector<Index *> *indexes_;
  bool has_pk = false;
  std::mutex table_mutex;
  std::atomic<int> max_pk_value_inserted;
  std::string engine;
};

/* Partition table */
struct Partition_table : public Table {
public:
  Partition_table(std::string n) : Table(n){};
  Partition_table(std::string n, int max_pk) : Table(n, max_pk){};
  ~Partition_table() {}
  std::string partition_start;
  //  string partiton_type() { return partition_start; }
};

/* Temporary table */
struct Temporary_table : public Table {
public:
  Temporary_table(std::string n) : Table(n){};
  Temporary_table(const Temporary_table &table) : Table(table.name_){};
};

int set_seed(Thd1 *thd);
int sum_of_all_options();
int sum_of_all_server_options();
Option::Opt pick_some_option();
std::vector<std::string> *random_strs_generator(unsigned long int seed);
bool run_default_load(Thd1 *thd);
void run_some_query(Thd1 *thd);
int save_dictionary(std::vector<Table *> *all_tables);
std::string rand_string(int upper, int lower = 0);
bool execute_sql(std::string sql, Thd1 *thd);
void load_default_data(Thd1 *thd);
void save_objects_to_file();
void load_objects_from_file();
void create_default_tables();
void clean_up_at_end();
void create_database_tablespace(Thd1 *thd);
Table *select_random_table();
void alter_tablespace_encryption(Thd1 *thd);
void alter_tablespace_rename(Thd1 *thd);
void set_mysqld_variable(Thd1 *thd);
#endif
