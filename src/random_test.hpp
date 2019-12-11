#ifndef __RANDOM_HPP__
#define __RANDOM_HPP__

#include "common.hpp"
#include <algorithm>
#include <atomic>
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
#define INNODB_16K_PAGE_SIZE 16
#define INNODB_8K_PAGE_SIZE 8
#define INNODB_32K_PAGE_SIZE 32
#define INNODB_64K_PAGE_SIZE 64
#define MIN_SEED_SIZE 10000
#define MAX_SEED_SIZE 100000
#define MAX_RANDOM_STRING_SIZE 32
#define DESC_INDEXES_IN_COLUMN 34
#define MYSQL_8 8.0

#define opt_int(a) options->at(Option::a)->getInt();
#define opt_int_set(a, b) options->at(Option::a)->setInt(b);
#define opt_bool(a) options->at(Option::a)->getBool();
#define opt_string(a) options->at(Option::a)->getString();

int rand_int(int upper, int lower = 0);
std::string rand_float(float upper, float lower = 0, int precision = 2);
std::string rand_string(int upper, int lower = 0);

struct Table;
class Column {
public:
  enum COLUMN_TYPES {
    INT,
    CHAR,
    VARCHAR,
    FLOAT,
    DOUBLE,
    BOOL,
    BLOB,
    GENERATED,
    COLUMN_MAX // should be last
  } type_;
  /* used to create new table/alter table add column*/
  Column(std::string name, Table *table, COLUMN_TYPES type);

  Column(Table *table, COLUMN_TYPES type) : type_(type), table_(table){};

  Column(std::string name, std::string type, Table *table)
      : type_(col_type(type)), name_(name), table_(table){};

  /* table definition */
  std::string definition();
  /* return random value of that column */
  virtual std::string rand_value();
  /* return string to call type */
  const std::string col_type_to_string(COLUMN_TYPES type) const;
  /* return column type from a string */
  COLUMN_TYPES col_type(std::string type);
  /* used to create_metadata */
  template <typename Writer> void Serialize(Writer &writer) const;
  /* return the clause of column */
private:
  virtual std::string clause() {
    std::string str;
    str = col_type_to_string(type_);
    if (length > 0)
      str += "(" + std::to_string(length) + ")";
    return str;
  };

public:
  virtual ~Column(){};
  std::string name_;
  std::mutex mutex;
  bool null = false;
  int length = 0;
  std::string default_value;
  bool primary_key = false;
  bool auto_increment = false;
  bool compressed = false; // percona type compressed
  Table *table_;
};

struct Blob_Column : public Column {
  Blob_Column(std::string name, Table *table);
  Blob_Column(std::string name, Table *table, std::string sub_type_);
  std::string sub_type; // sub_type can be tiny, medium, large blob
  std::string clause() { return sub_type; };
  std::string rand_value() { return "\'" + rand_string(1000) + "\'"; }
  template <typename Writer> void Serialize(Writer &writer) const;
};

struct Generated_Column : public Column {

  /* constructor for new random generated column */
  Generated_Column(std::string name, Table *table);

  /* constructor used to prepare metadata */
  Generated_Column(std::string name, Table *table, std::string clause,
                   std::string sub_type);

  template <typename Writer> void Serialize(Writer &writer) const;

  std::string str;
  std::string clause() { return str; };
  std::string rand_value() { return "default"; };
  ~Generated_Column(){};
  COLUMN_TYPES g_type; // sub type can be blob,int, varchar
  COLUMN_TYPES generate_type() { return g_type; };
};

struct Ind_col {
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

  std::string definition();

  std::string name_;
  std::vector<Ind_col *> *columns_;
};

struct Thd1 {
  Thd1(int id, std::ofstream &tl, std::ofstream &ddl_l, std::ofstream &client_l,
       MYSQL *c, std::atomic<unsigned long long> &p,
       std::atomic<unsigned long long> &f)
      : thread_id(id), thread_log(tl), ddl_logs(ddl_l), client_log(client_l),
        conn(c), performed_queries_total(p), failed_queries_total(f){};

  void run_some_query(); // create default tables and run random queries
  bool load_metadata();  // load metada of tool in memory

  int thread_id;
  int seed;
  std::ofstream &thread_log;
  std::ofstream &ddl_logs;
  std::ofstream &client_log;
  MYSQL *conn;
  std::atomic<unsigned long long> &performed_queries_total;
  std::atomic<unsigned long long> &failed_queries_total;
  std::string result;         // resul of sql
  bool ddl_query = false;     // is the query ddl
  bool success = false;       // if the sql is successfully executed
  bool store_result = false;  // store result of executed sql
  int max_con_fail_count = 0; // consecutive failed queries
  int query_number = 0;
};

/* Table basic properties */
struct Table {
public:
  enum TABLE_TYPES { PARTITION, NORMAL, TEMPORARY, TABLE_MAX } type;

  Table(std::string n);
  static Table *table_id(TABLE_TYPES choice, int id, Thd1 *thd);
  std::string definition();
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
  void SetTableCompression(Thd1 *thd);
  void ModifyColumn(Thd1 *thd);
  void InsertRandomRow(Thd1 *thd, bool islock);
  void DropColumn(Thd1 *thd);
  void AddColumn(Thd1 *thd);
  void DropIndex(Thd1 *thd);
  void AddIndex(Thd1 *thd);
  void DeleteRandomRow(Thd1 *thd);
  void UpdateRandomROW(Thd1 *thd);
  void SelectRandomRow(Thd1 *thd);
  void SelectAllRow(Thd1 *thd);
  void DeleteAllRows(Thd1 *thd);
  void ColumnRename(Thd1 *thd);
  template <typename Writer> void Serialize(Writer &writer) const;
  virtual ~Table();

  std::string name_;
  std::string engine;
  std::string row_format;
  std::string tablespace;
  std::string compression;
  bool encryption = false;
  int key_block_size = 0;
  // std::string data_directory; todo add corressponding code
  std::vector<Column *> *columns_;
  std::vector<Index *> *indexes_;
  std::mutex table_mutex;
};

/* Partition table */
struct Partition_table : public Table {
public:
  Partition_table(std::string n) : Table(n){};
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
int sum_of_all_options(Thd1 *thd);
int sum_of_all_server_options();
Option::Opt pick_some_option();
std::vector<std::string> *random_strs_generator(unsigned long int seed);
bool load_metadata(Thd1 *thd);
void run_some_query(Thd1 *thd);
int save_dictionary(std::vector<Table *> *all_tables);
bool execute_sql(std::string sql, Thd1 *thd);
void load_default_data(Table *table, Thd1 *thd);
void save_metadata_to_file();
void clean_up_at_end();
void alter_tablespace_encryption(Thd1 *thd);
void alter_tablespace_rename(Thd1 *thd);
void set_mysqld_variable(Thd1 *thd);
void add_server_options(std::string str);
void alter_database_encryption(Thd1 *thd);
void create_in_memory_data();
void create_default_tables(Thd1 *thd);
void create_database_tablespace(Thd1 *thd);
#endif
