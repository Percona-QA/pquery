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

enum RANDOM_SQL {
  ADD_COLUMN = 20,
  INSERT_RANDOM_ROW,
  DROP_CREATE,
  OPTIMIZE,
  ANALYZE,
  DELETE_ALL_ROW,
  DELETE_ROW_USING_PKEY,
  UPDATE_ROW_USING_PKEY,
  SELECT_ROW_USING_PKEY,
  SELECT_ALL_ROW,
  COLUMN_RENAME,
  // DELETE_ROW_RANDOM,
  // UPDATE_ROW_USING_PKEY
  RANDOM_MAX
};
/* Different table type supported by tool */
enum TABLE_TYPES { PARTITION, NORMAL, TABLE_MAX };
/* Column Basic Properties */


int rand_int(int upper, int lower = 0);

struct Table;

struct Column {
  Column(std::string name, Table *table);
  Column(std::string name, std::string type, bool is_null, int len,
         Table *table);
  Column(std::string name, Table *table, int type);
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
  Thd1(int id, std::ofstream &tl, MYSQL *c, std::vector<Table *> *tab)
      : thread_id(id), thread_log(tl), conn(c), tables(tab){};

  int thread_id;
  std::ofstream &thread_log;
  MYSQL *conn;
  std::vector<Table *> *tables;
  int seed;
  static std::vector<std::string> *random_strs;
  static std::vector<std::string> encryption;
  static std::vector<std::string> row_format;
  static std::vector<int> key_block_size;
  static std::vector<std::string> tablespace;
  static int pkey_pb_per_table;
  static int s_len;
  static bool is_innodb_system_encrypted;
  static int max_columns_length;
  static int max_columns_in_table;
  static int max_indexes_in_table;
  static int max_columns_in_index;
  static int innodb_page_size;
};


/* Table basic properties */
struct Table {
public:
  TABLE_TYPES table_type;
  Table(std::string n, int max_pk);

  static Table *table_id(int choice, int id);
  std::string Table_defination();
  /* methods to create table of choice */
  void AddInternalColumn(Column *column) { columns_->push_back(column); }
  void AddInternalIndex(Index *index) { indexes_->push_back(index); }
  void CreateDefaultColumn();
  void CreateDefaultIndex();
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
  Partition_table(std::string n, int max_pk) : Table(n, max_pk){};
  ~Partition_table() {}
  std::string partition_start;
  //  string partiton_type() { return partition_start; }
};

void create_database_tablespace(Thd1 *thd);
int set_seed(Thd1 *thd);
int run_default_load(Thd1 *thd);
void run_some_query(Thd1 *thd);
void alter_tablespace_encryption(Thd1 *thd);
void alter_tablespace_rename(Thd1 *thd);
int save_dictionary(std::vector<Table *> *all_tables);
std::string rand_string(int upper, int lower = 0);
int execute_sql(std::string sql, Thd1 *thd);
void load_default_data(std::vector<Table *> *all_tables, Thd1 *thd);
void save_objects_to_file(std::vector<Table *> *all_tables);
void load_objects_from_file(std::vector<Table *> *all_tables);
void create_default_tables(std::vector<Table *> *all_tables);
void clean_up_at_end(std::vector<Table *> *all_tables);
int sum_of_all_options();
Option::Opt pick_some_option();
std::vector<std::string> *random_strs_generator(unsigned long int seed);
#endif
