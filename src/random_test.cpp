/*
 =========================================================
 #       Created by Rahul Malik, Percona LLC             #
 =========================================================
*/
#include "random_test.hpp"
#include "common.hpp"
#include "node.hpp"
#include <iomanip>
#include <regex>
#include <sstream>

using namespace rapidjson;
std::mt19937 rng;

const std::string partition_string = "_p";
const int version = 1;

static std::vector<Table *> *all_tables = new std::vector<Table *>;
static std::vector<std::string> g_undo_tablespace;
static std::vector<std::string> g_encryption;
static std::vector<std::string> g_compression = {"none", "zlib", "lz4"};
static std::vector<std::string> g_row_format;
static std::vector<std::string> g_tablespace;
static std::vector<int> g_key_block_size;
static int g_max_columns_length = 30;
static int g_innodb_page_size;
static int sum_of_all_opts = 0; // sum of all probablility
std::mutex ddl_logs_write;
static std::chrono::system_clock::time_point start_time =
    std::chrono::system_clock::now();

std::atomic<size_t> table_started(0);
std::atomic<size_t> table_completed(0);
std::atomic_flag lock_stream = ATOMIC_FLAG_INIT;
std::atomic<bool> connection_lost(false);

/* get result of sql */
static std::string get_result(std::string sql, Thd1 *thd) {
  thd->store_result = true;
  execute_sql(sql, thd);
  auto result = thd->result;
  thd->result = "";
  thd->store_result = false;
  return result;
}

/* return the db branch */
static std::string db_branch() {
  std::string branch = mysql_get_client_info();
  branch = branch.substr(0, 3); // 8.0 or 5.7
  return branch;
}

/* return probabality of all options and disable some feature based on user
 * request/ branch/ fork */
int sum_of_all_options(Thd1 *thd) {

  /* for 5.7 disable some features */
  if (db_branch().compare("5.7") == 0) {
    opt_int_set(ALTER_TABLESPACE_RENAME, 0);
    opt_int_set(RENAME_COLUMN, 0);
    opt_int_set(UNDO_SQL, 0);
    opt_int_set(ALTER_TABLE_ENCRYPTION_INPLACE, 0);
  }

  auto enc_type = options->at(Option::ENCRYPTION_TYPE)->getString();

  /* for percona-server we have additional encryption type keyring */
  if (enc_type.compare("all") == 0) {
    g_encryption = {"Y", "N"};
    if (strcmp(FORK, "Percona-Server") == 0) {
      g_encryption.push_back("KEYRING");
    }
  } else if (enc_type.compare("oracle") == 0)
    g_encryption = {"Y", "N"};
  else
    g_encryption = {enc_type};

  /* feature not supported by oracle */
  if (strcmp(FORK, "MySQL") == 0) {
    options->at(Option::ALTER_DATABASE_ENCRYPTION)->setInt(0);
    options->at(Option::NO_COLUMN_COMPRESSION)->setBool("true");
    opt_int_set(ALTER_TABLE_ENCRYPTION_INPLACE, 0);
  }

  if (db_branch().compare("8.0") == 0) {
    /* for 8.0 default columns set default colums */
    if (!options->at(Option::COLUMNS)->cl)
      options->at(Option::COLUMNS)->setInt(7);
  }

  /* if select is set as zero, disable all type of selects */
  if (options->at(Option::NO_SELECT)->getBool()) {
    options->at(Option::SELECT_ALL_ROW)->setInt(0);
    options->at(Option::SELECT_ROW_USING_PKEY)->setInt(0);
  }
  /* if delete is set as zero, disable all type of deletes */
  if (options->at(Option::NO_DELETE)->getBool()) {
    options->at(Option::DELETE_ALL_ROW)->setInt(0);
    options->at(Option::DELETE_ROW_USING_PKEY)->setInt(0);
  }
  /* If update is disable, set all update probability to zero */
  if (options->at(Option::NO_UPDATE)->getBool()) {
    options->at(Option::UPDATE_ROW_USING_PKEY)->setInt(0);
  }
  /* if insert is disable, set all insert probability to zero */
  if (options->at(Option::NO_INSERT)->getBool()) {
    opt_int_set(INSERT_RANDOM_ROW, 0);
  }
  /* if no-tbs, do not execute tablespace related sql */
  if (options->at(Option::NO_TABLESPACE)->getBool()) {
    opt_int_set(ALTER_TABLESPACE_RENAME, 0);
    opt_int_set(ALTER_TABLESPACE_ENCRYPTION, 0);
  }
  /* If no-encryption is set, disable all encryption options */
  if (options->at(Option::NO_ENCRYPTION)->getBool()) {
    opt_int_set(ALTER_TABLE_ENCRYPTION, 0);
    opt_int_set(ALTER_TABLE_ENCRYPTION_INPLACE, 0);
    opt_int_set(ALTER_TABLESPACE_ENCRYPTION, 0);
    opt_int_set(ALTER_MASTER_KEY, 0);
    opt_int_set(ROTATE_REDO_LOG_KEY, 0);
    opt_int_set(ALTER_DATABASE_ENCRYPTION, 0);
  }

  /* If no-table-encryption is set, disable all compression */
  if (options->at(Option::NO_TABLE_COMPRESSION)->getBool()) {
    opt_int_set(ALTER_TABLE_COMPRESSION, 0);
    g_compression.clear();
  }

  /* if no dynamic variables is passed set-global to zero */
  if (server_options->empty())
    opt_int_set(SET_GLOBAL_VARIABLE, 0);

  auto only_cl_ddl = opt_bool(ONLY_CL_DDL);
  auto only_cl_sql = opt_bool(ONLY_CL_SQL);
  auto no_ddl = opt_bool(NO_DDL);

  /* if set, then disable all other DDL */
  if (only_cl_sql) {
    for (auto &opt : *options) {
      if (opt != nullptr && opt->sql && !opt->cl)
        opt->setInt(0);
    }
  }

  /* only-cl-ddl, if set then disable all other DDL */
  if (only_cl_ddl) {
    for (auto &opt : *options) {
      if (opt != nullptr && opt->ddl && !opt->cl)
        opt->setInt(0);
    }
  }

  if (only_cl_ddl && no_ddl)
    throw std::runtime_error("noddl && only-cl-ddl can't be passed together");

  /* if no ddl is set disable all ddl */
  if (no_ddl) {
    for (auto &opt : *options) {
      if (opt != nullptr && opt->sql && opt->ddl)
        opt->setInt(0);
    }
  }

  int total = 0;
  for (auto &opt : *options) {
    if (opt == nullptr)
      continue;
    if (opt->getType() == Option::INT)
      thd->thread_log << opt->getName() << "=>" << opt->getInt() << std::endl;
    else if(opt->getType() == Option::BOOL)
      thd->thread_log << opt->getName() << "=>" << opt->getBool() << std::endl;
    if (!opt->sql)
      continue;
    total += opt->getInt();
  }

  return total;
}

/* return some options */
Option::Opt pick_some_option() {
  int rd = rand_int(sum_of_all_opts, 1);
  for (auto &opt : *options) {
    if (opt == nullptr || !opt->sql)
      continue;
    if (rd <= opt->getInt())
      return opt->getOption();
    else
      rd -= opt->getInt();
  }
  return Option::MAX;
}

int sum_of_all_server_options() {
  int total = 0;
  for (auto &opt : *server_options) {
    total += opt->prob;
  }
  return total;
}

/* pick some algorith */
inline static std::string pick_algorithm_lock() {
  /* pick algorith for current sql */
  static auto lock = opt_string(LOCK);
  static auto algorithm = opt_string(ALGORITHM);
  std::string locks[] = {"DEFAULT", "EXCLUSIVE", "SHARED", "NONE"};
  std::string algorithms[] = {"INPLACE", "COPY", "DEFAULT"};
  std::string current_lock;
  std::string current_algo;
  if (lock.compare("all") == 0 && algorithm.compare("all") == 0) {
    auto lock_index = rand_int(3);
    auto algo_index = rand_int(2);
    /* lock=none;algo=inplace|copy not supported */
    if (lock_index == 3 && algo_index != 2)
      lock_index = 0;
    current_lock = locks[lock_index];
    current_algo = algorithms[algo_index];
  } else if (lock.compare("all") == 0) {
    auto lock_index = rand_int(3);
    current_lock = locks[lock_index];
    current_algo = algorithm;
  } else if (algorithm.compare("all") == 0) {
    auto algo_index = rand_int(2);
    current_algo = algorithms[algo_index];
    current_lock = lock;
  } else {
    current_lock = lock;
    current_algo = algorithm;
  }
  return ", LOCK=" + current_lock + ", ALGORITHM=" + current_algo;
}

/* set seed of current thread */
int set_seed(Thd1 *thd) {
  auto initial_seed = opt_int(INITIAL_SEED);
  rng = std::mt19937(initial_seed);
  thd->thread_log << "Initial seed " << initial_seed << std::endl;
  for (int i = 0; i < thd->thread_id; i++)
    rand_int(MIN_SEED_SIZE, MAX_SEED_SIZE);
  thd->seed = rand_int(MAX_SEED_SIZE, MIN_SEED_SIZE);
  thd->thread_log << "CURRENT SEED IS " << thd->seed << std::endl;
  return thd->seed;
}

/* generate random strings of size N_STR */
std::vector<std::string> *random_strs_generator(unsigned long int seed) {
  static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "0123456789";

  static const size_t N_STRS = 10000;

  std::default_random_engine rng(seed);
  std::uniform_int_distribution<> dist(0, sizeof(alphabet) / sizeof(*alphabet) -
                                              2);

  std::vector<std::string> *strs = new std::vector<std::string>;
  strs->reserve(N_STRS);
  std::generate_n(std::back_inserter(*strs), strs->capacity(), [&] {
    std::string str;
    str.reserve(MAX_RANDOM_STRING_SIZE);
    std::generate_n(std::back_inserter(str), MAX_RANDOM_STRING_SIZE,
                    [&]() { return alphabet[dist(rng)]; });

    return str;
  });
  return strs;
}

std::vector<std::string> *random_strs;

int rand_int(int upper, int lower) {
  /*todo change the approach if it is too slow */
  std::uniform_int_distribution<std::mt19937::result_type> dist(
      lower, upper); // distribution in range [lower, upper]
  return dist(rng);
}

/* return random float number in the range of upper and lower */
std::string rand_float(float upper, float lower) {
  static std::uniform_real_distribution<> dis(lower, upper);
  std::ostringstream out;
  out << std::fixed;
  out << std::setprecision(2) << (float)(dis(rng));
  return out.str();
}

std::string rand_double(double upper, double lower) {
  static std::uniform_real_distribution<> dis(lower, upper);
  std::ostringstream out;
  out << std::fixed;
  out << std::setprecision(5) << (double)(dis(rng));
  return out.str();
}

/* return random string in range of upper and lower */
std::string rand_string(int upper, int lower) {
  std::string rs = ""; /*random_string*/
  auto size = rand_int(upper, lower);

  /* let the query fail with string size greater */
  if (rand_int(100) < 3)
    size *= 1;

  while (size > 0) {
    auto str = random_strs->at(rand_int(random_strs->size() - 1));
    if (size > MAX_RANDOM_STRING_SIZE)
      rs += str;
    else
      rs += str.substr(0, size);
    size -= MAX_RANDOM_STRING_SIZE;
  }
  return rs;
}

/* return column type from a string */
Column::COLUMN_TYPES Column::col_type(std::string type) {
  if (type.compare("INT") == 0)
    return INT;
  else if (type.compare("CHAR") == 0)
    return CHAR;
  else if (type.compare("VARCHAR") == 0)
    return VARCHAR;
  else if (type.compare("BOOL") == 0)
    return BOOL;
  else if (type.compare("GENERATED") == 0)
    return GENERATED;
  else if (type.compare("BLOB") == 0)
    return BLOB;
  else if (type.compare("FLOAT") == 0)
    return FLOAT;
  else if (type.compare("DOUBLE") == 0)
    return DOUBLE;
  else
    throw std::runtime_error("unhandled " + col_type_to_string(type_) +
                             " at line " + std::to_string(__LINE__));
}

/* return string from a column type */
const std::string Column::col_type_to_string(COLUMN_TYPES type) const {
  switch (type) {
  case INT:
    return "INT";
  case CHAR:
    return "CHAR";
  case DOUBLE:
    return "DOUBLE";
  case FLOAT:
    return "FLOAT";
  case VARCHAR:
    return "VARCHAR";
  case BOOL:
    return "BOOL";
  case BLOB:
    return "BLOB";
  case GENERATED:
    return "GENERATED";
  case COLUMN_MAX:
    break;
  }
  return "FAIL";
}

/* return random value of any string */
std::string Column::rand_value() {
  switch (type_) {
  case (COLUMN_TYPES::INT):
    static auto rec = 100 * opt_int(INITIAL_RECORDS_IN_TABLE);
    return std::to_string(rand_int(rec));
    break;
  case (COLUMN_TYPES::FLOAT):
  {
    static float rec1 = 0.01 * opt_int(INITIAL_RECORDS_IN_TABLE);
    return rand_float(rec1);
    break;
  }
  case (COLUMN_TYPES::DOUBLE):
  {
    static float rec2 = 0.00001 * opt_int(INITIAL_RECORDS_IN_TABLE);
    return rand_double(rec2);
    break;
  }
  case CHAR:
  case VARCHAR:
    return "\'" + rand_string(length) + "\'";
  case BOOL:
    return (rand_int(1) == 1 ? "true" : "false");
    break;
  case BLOB:
  case GENERATED:
  case COLUMN_MAX:
    throw std::runtime_error("unhandled " + col_type_to_string(type_) +
                             " at line " + std::to_string(__LINE__));
  }
  return "";
}

/* return table definition */
std::string Column::definition() {
  std::string def = name_ + " " + clause();
  if (null)
    def += " NOT NULL";
  if (auto_increment)
    def += " AUTO_INCREMENT";
  if (compressed) {
    def += " COLUMN_FORMAT COMPRESSED";
  }
  return def;
}

/* add new column, part of create table or Alter table */
Column::Column(std::string name, Table *table, COLUMN_TYPES type)
    : table_(table) {
  type_ = type;
  switch (type) {
  case CHAR:
    name_ = "c" + name;
    length = rand_int(g_max_columns_length, 10);
    break;
  case VARCHAR:
    name_ = "v" + name;
    length = rand_int(g_max_columns_length, 10);
    break;
  case INT:
    name_ = "i" + name;
    if (rand_int(10) == 1)
      length = rand_int(100, 20);
    break;
  case FLOAT:
    name_ = "f" + name;
    break;
  case DOUBLE:
    name_ = "d" + name;
    break;
  case BOOL:
    name_ = "t" + name;
    break;
  default:
    throw std::runtime_error("unhandled " + col_type_to_string(type_) +
                             " at line " + std::to_string(__LINE__));
  }
}

/* add new blob column, part of create table or Alter table */
Blob_Column::Blob_Column(std::string name, Table *table)
    : Column(table, Column::BLOB) {

  if (options->at(Option::NO_COLUMN_COMPRESSION)->getBool() == false &&
      rand_int(1) == 1)
    compressed = true;
  switch (rand_int(5, 1)) {
  case 1:
    sub_type = "MEDIUMTEXT";
    name_ = "mt" + name;
    break;
  case 2:
    sub_type = "TEXT";
    name_ = "t" + name;
    break;
  case 3:
    sub_type = "LONGTEXT";
    name_ = "lt" + name;
    break;
  case 4:
    sub_type = "BLOB";
    name_ = "b" + name;
    break;
  case 5:
    sub_type = "LONGBLOB";
    name_ = "lb" + name;
    break;
  }
}

Blob_Column::Blob_Column(std::string name, Table *table, std::string sub_type_)
    : Column(table, Column::BLOB) {
  name_ = name;
  sub_type = sub_type_;
}

/* Constructor used for load metadata */
Generated_Column::Generated_Column(std::string name, Table *table,
                                   std::string clause, std::string sub_type)
    : Column(table, Column::GENERATED) {
  name_ = name;
  str = clause;
  g_type = Column::col_type(sub_type);
}

/* Generated column constructor. lock table before calling */
Generated_Column::Generated_Column(std::string name, Table *table)
    : Column(table, Column::GENERATED) {
  name_ = "g" + name;
  auto blob_supported = !options->at(Option::NO_BLOB)->getBool();
  g_type = COLUMN_MAX;
  /* Generated columns are 2:2:2:2 (INT:VARCHAR:CHAR:BLOB) */
  while (g_type == COLUMN_MAX) {
    auto x = rand_int(4, 1);
    if (x <= 1)
      g_type = INT;
    if (x <= 2)
      g_type = VARCHAR;
    else if (x <= 3)
      g_type = CHAR;
    else if (blob_supported && x <= 4) {
      g_type = BLOB;
    }
  }

  if (options->at(Option::NO_COLUMN_COMPRESSION)->getBool() == false &&
      rand_int(1) == 1 && g_type == BLOB)
    compressed = true;

  /*number of columns in generated columns */
  size_t columns = rand_int(.6 * table->columns_->size()) + 1;

  std::vector<size_t> col_pos; // position of columns
  while (col_pos.size() < columns) {
    size_t col = rand_int(table->columns_->size() - 1);
    if (!table->columns_->at(col)->auto_increment &&
        table->columns_->at(col)->type_ != GENERATED)
      col_pos.push_back(col);
  }

  if (g_type == INT) {
    str = " " + col_type_to_string(g_type) + " AS(";
    for (auto pos : col_pos) {
      auto col = table->columns_->at(pos);
      if (col->type_ == VARCHAR || col->type_ == CHAR || col->type_ == BLOB)
        str += " LENGTH(" + col->name_ + ")+";
      else if (col->type_ == INT || col->type_ == BOOL)
        str += " " + col->name_ + "+";
      else
        throw std::runtime_error("unhandled " + col_type_to_string(col->type_) +
                                 " at line " + std::to_string(__LINE__));
    }
    str.pop_back();
  } else if (g_type == VARCHAR || g_type == CHAR || g_type == BLOB) {
    auto size = rand_int(g_max_columns_length, col_pos.size());
    int actual_size = 0;
    std::string gen_sql;
    for (auto pos : col_pos) {
      auto col = table->columns_->at(pos);
      auto current_size = rand_int((int)size / col_pos.size() * 2, 1);
      int column_size = 0;
      /* base column */
      switch (col->type_) {
      case INT:
        column_size = 10; // interger max string size is 10
        break;
      case FLOAT:
      case DOUBLE:
        column_size = 10;
        break;
      case BOOL:
        column_size = 1;
        break;
      case VARCHAR:
      case CHAR:
        column_size = col->length;
        break;
      case BLOB:
        column_size = 5000; // todo set it different subtype
        break;
      case COLUMN_MAX:
      case GENERATED:
        throw std::runtime_error("unhandled " + col_type_to_string(col->type_) +
                                 " at line " + std::to_string(__LINE__));
      }
      if (column_size > current_size) {
        actual_size += current_size;
        gen_sql +=
            "SUBSTRING(" + col->name_ + ",1," + std::to_string(current_size) + "),";
      } else {
        actual_size += column_size;
        gen_sql += col->name_ + ",";
      }
    }
    gen_sql.pop_back();
    str = " " + col_type_to_string(g_type);
    if (g_type == VARCHAR || g_type == CHAR)
      str += "(" + std::to_string(actual_size) + ")";
    str += " AS  (CONCAT(";
    str += gen_sql;
    str += ")";
    length = actual_size;
  } else {
    throw std::runtime_error("unhandled " + col_type_to_string(g_type) +
                             " at line " + std::to_string(__LINE__));
  }
  str += ")";

  if (rand_int(2) == 1 || compressed)
    str += " STORED";
}

template <typename Writer> void Column::Serialize(Writer &writer) const {
  writer.String("name");
  writer.String(name_.c_str(), static_cast<SizeType>(name_.length()));
  writer.String("type");
  std::string typ = col_type_to_string(type_);
  writer.String(typ.c_str(), static_cast<SizeType>(typ.length()));
  writer.String("null");
  writer.Bool(null);
  writer.String("primary_key");
  writer.Bool(primary_key);
  writer.String("compressed");
  writer.Bool(compressed);
  writer.String("auto_increment");
  writer.Bool(auto_increment);
  writer.String("lenght");
  writer.Int(length);
}

/* add sub_type metadata */
template <typename Writer> void Blob_Column::Serialize(Writer &writer) const {
  writer.String("sub_type");
  writer.String(sub_type.c_str(), static_cast<SizeType>(sub_type.length()));
}

/* add sub_type and clause in metadata */
template <typename Writer>
void Generated_Column::Serialize(Writer &writer) const {
  writer.String("sub_type");
  auto type = col_type_to_string(g_type);
  writer.String(type.c_str(), static_cast<SizeType>(type.length()));
  writer.String("clause");
  writer.String(str.c_str(), static_cast<SizeType>(str.length()));
}

template <typename Writer> void Ind_col::Serialize(Writer &writer) const {
  writer.StartObject();
  writer.String("name");
  auto &name = column->name_;
  writer.String(name.c_str(), static_cast<SizeType>(name.length()));
  writer.String("desc");
  writer.Bool(desc);
  writer.String("length");
  writer.Uint(length);
  writer.EndObject();
}

template <typename Writer> void Index::Serialize(Writer &writer) const {
  writer.StartObject();
  writer.String("name");
  writer.String(name_.c_str(), static_cast<SizeType>(name_.length()));
  writer.String(("index_columns"));
  writer.StartArray();
  for (auto ic : *columns_)
    ic->Serialize(writer);
  writer.EndArray();
  writer.EndObject();
}

Index::~Index() {
  for (auto id_col : *columns_) {
    delete id_col;
  }
  delete columns_;
}

template <typename Writer> void Table::Serialize(Writer &writer) const {
  writer.StartObject();

  writer.String("name");
  writer.String(name_.c_str(), static_cast<SizeType>(name_.length()));

  writer.String("engine");
  if (!engine.empty())
    writer.String(engine.c_str(), static_cast<SizeType>(engine.length()));
  else
    writer.String("default");

  writer.String("row_format");
  if (!row_format.empty())
    writer.String(row_format.c_str(),
                  static_cast<SizeType>(row_format.length()));
  else
    writer.String("default");

  writer.String("tablespace");
  if (!tablespace.empty())
    writer.String(tablespace.c_str(),
                  static_cast<SizeType>(tablespace.length()));
  else
    writer.String("file_per_table");

  writer.String("encryption");
  writer.String(encryption.c_str(), static_cast<SizeType>(encryption.length()));

  writer.String("compression");
  writer.String(compression.c_str(),
                static_cast<SizeType>(compression.length()));

  writer.String("key_block_size");
  writer.Int(key_block_size);

  writer.String(("columns"));
  writer.StartArray();

  /* write all colummns */
  for (auto &col : *columns_) {
    writer.StartObject();
    col->Serialize(writer);
    if (col->type_ == Column::GENERATED) {
      static_cast<Generated_Column *>(col)->Serialize(writer);
    } else if (col->type_ == Column::BLOB) {
      static_cast<Blob_Column *>(col)->Serialize(writer);
    }
    writer.EndObject();
  }

  writer.EndArray();

  writer.String(("indexes"));
  writer.StartArray();
  for (auto *ind : *indexes_)
    ind->Serialize(writer);
  writer.EndArray();
  writer.EndObject();
}

Ind_col::Ind_col(Column *c, bool d) : column(c), desc(d) {}

Index::Index(std::string n) : name_(n), columns_() {
  columns_ = new std::vector<Ind_col *>;
}

void Index::AddInternalColumn(Ind_col *column) { columns_->push_back(column); }

/* index definition */
std::string Index::definition() {
  std::string def;
  def += "INDEX " + name_ + "(";
  for (auto idc : *columns_) {
    def += idc->column->name_;

    /* blob columns should have prefix length */
    if (idc->column->type_ == Column::BLOB ||
        (idc->column->type_ == Column::GENERATED &&
         static_cast<Generated_Column *>(idc->column)->generate_type() ==
             Column::BLOB))
      def += "(" + std::to_string(rand_int(g_max_columns_length, 1)) + ")";

    def += (idc->desc ? " DESC" : (rand_int(3) ? "" : " ASC"));
    def += ", ";
  }
  def.erase(def.length() - 2);
  def += ") ";
  return def;
}

Table::Table(std::string n) : name_(n), indexes_() {
  columns_ = new std::vector<Column *>;
  indexes_ = new std::vector<Index *>;
}

void Table::DropCreate(Thd1 *thd) {
  execute_sql("DROP TABLE " + name_, thd);
  std::string def = definition();
  if (!execute_sql(def, thd) && tablespace.size() > 0) {
    std::string tbs = " TABLESPACE=" + tablespace + "_rename";

    auto no_encryption = opt_bool(NO_ENCRYPTION);

    std::string encrypt_sql = " ENCRYPTION = " + encryption;

    /* If tablespace is rename or encrypted, or tablespace rename/encrypted */
    if (!execute_sql(def + tbs, thd))
      if (!no_encryption && (execute_sql(def + encrypt_sql, thd) ||
                             execute_sql(def + encrypt_sql + tbs, thd))) {
        table_mutex.lock();
        if (encryption.compare("Y") == 0)
          encryption = 'N';
        else if (encryption.compare("N") == 0)
          encryption = 'Y';
        table_mutex.unlock();
      }
  }
}

void Table::Optimize(Thd1 *thd) { execute_sql("OPTIMIZE TABLE " + name_, thd); }

void Table::Analyze(Thd1 *thd) { execute_sql("ANALYZE TABLE " + name_, thd); }

void Table::Truncate(Thd1 *thd) { execute_sql("TRUNCATE TABLE " + name_, thd); }

Table::~Table() {
  for (auto ind : *indexes_)
    delete ind;
  for (auto col : *columns_) {
    col->mutex.lock();
    delete col;
  }
  delete columns_;
  delete indexes_;
}

/* create default column */
void Table::CreateDefaultColumn() {
  auto no_auto_inc = opt_bool(NO_AUTO_INC);

  /* create normal column */
  static auto max_col = opt_int(COLUMNS);

  auto max_columns = rand_int(max_col, 1);
  bool auto_increment = false;

  for (int i = 0; i < max_columns; i++) {
    std::string name;
    Column::COLUMN_TYPES type;
    Column *col;
    /*  if we need to create primary column */
    static int pkey_pb_per_table = opt_int(PRIMARY_KEY);

    /* First column can be primary */
    if (i == 0 && rand_int(100) < pkey_pb_per_table) {
      type = Column::INT;
      name = "pkey";
      col = new Column{name, this, type};
      col->primary_key = true;
      if (!no_auto_inc &&
          rand_int(3) < 3) { /* 75% of primary key tables are autoinc */
        col->auto_increment = true;
        auto_increment = true;
      }

    } else {
      name = std::to_string(i);

      Column::COLUMN_TYPES col_type = Column::COLUMN_MAX;
      static auto no_virtual_col = opt_bool(NO_VIRTUAL_COLUMNS);
      static auto no_blob_col = opt_bool(NO_BLOB);

      /* loop untill we select some column */
      while (col_type == Column::COLUMN_MAX) {

        /* columns are 6:2:2:4:2:2:1 INT:FLOAT:DOUBLE:VARCHAR:CHAR:BLOB:BOOL */
        auto prob = rand_int(19);

        /* intial columns can't be generated columns. also 50% of tables last
         * columns are virtuals */
        if (!no_virtual_col && i >= .8 * max_columns && rand_int(1) == 1)
          col_type = Column::GENERATED;
        else if (prob < 6)
          col_type = Column::INT;
        else if (prob < 8)
          col_type = Column::FLOAT;
        else if (prob < 10)
          col_type = Column::DOUBLE;
        else if (prob < 14)
          col_type = Column::VARCHAR;
        else if (prob < 16)
          col_type = Column::CHAR;
        else if (!no_blob_col && prob < 18)
          col_type = Column::BLOB;
        else if (prob == 19)
          col_type = Column::BOOL;
      }

      if (col_type == Column::GENERATED)
        col = new Generated_Column(name, this);
      else if (col_type == Column::BLOB)
        col = new Blob_Column(name, this);
      else
        col = new Column(name, this, col_type);

      /* 25% column can have auto_inc */
      if (col->type_ == Column::INT && !no_auto_inc &&
          auto_increment == false && rand_int(100) > 25) {
        col->auto_increment = true;
        auto_increment = true;
      }
    }
    AddInternalColumn(col);
  }
}

/* create default indexes */
void Table::CreateDefaultIndex() {

  int auto_inc_pos = -1; // auto_inc_column_position

  static size_t max_indexes = opt_int(INDEXES);

  if (max_indexes == 0)
    return;

  /* if table have few column, decrease number of indexes */
  int indexes = rand_int(
      columns_->size() < max_indexes ? columns_->size() : max_indexes, 1);

  /* for auto-inc columns handling, we need to add auto_inc as first column */
  for (size_t i = 0; i < columns_->size(); i++) {
    if (columns_->at(i)->auto_increment) {
      auto_inc_pos = i;
    }
  }

  /*which column will hve auto_inc */
  int auto_inc_index = rand_int(indexes - 1, 0);

  for (int i = 0; i < indexes; i++) {
    Index *id = new Index(name_ + "i" + std::to_string(i));

    static size_t max_columns = opt_int(INDEX_COLUMNS);

    int number_of_compressed = 0;

    for (auto column : *columns_)
      if (column->compressed)
        number_of_compressed++;

    size_t number_of_columns = columns_->size() - number_of_compressed;

    /* only compressed columns */
    if (number_of_columns == 0)
      return;

    number_of_columns = rand_int(
        (max_columns < number_of_columns ? max_columns : number_of_columns), 1);

    std::vector<int> col_pos; // position of columns

    /* pick some columns */
    while (col_pos.size() < number_of_columns) {
      int current = rand_int(columns_->size() - 1);
      if (columns_->at(current)->compressed)
        continue;
      /* auto-inc column should be first column in auto_inc_index */
      if (auto_inc_pos != -1 && i == auto_inc_index && col_pos.size() == 0)
        col_pos.push_back(auto_inc_pos);
      else {
        bool already_added = false;
        for (auto id : col_pos) {
          if (id == current)
            already_added = true;
        }
        if (!already_added)
          col_pos.push_back(current);
      }
    } // while

    for (auto pos : col_pos) {
      auto col = columns_->at(pos);
      static bool no_desc_support = opt_bool(NO_DESC_INDEX);
      bool column_desc = false;
      if (!no_desc_support) {
        column_desc = rand_int(100) < DESC_INDEXES_IN_COLUMN
                          ? true
                          : false; // 33 % are desc //
      }
      id->AddInternalColumn(
          new Ind_col(col, column_desc)); // desc is set as true
    }
    AddInternalIndex(id);
  }
}

/* Create new table and pick some attributes */
Table *Table::table_id(TABLE_TYPES type, int id, Thd1 *thd) {
  Table *table;
  std::string name = "tt_" + std::to_string(id);
  switch (type) {
  case PARTITION:
    table = new Partition_table(name + partition_string);
    break;
  case NORMAL:
    table = new Table(name);
    break;
  case TEMPORARY:
    table = new Temporary_table(name + "_t");
    break;
  default:
    throw std::runtime_error("Unhandle Table type");
    break;
  }

  table->type = type;

  static auto no_encryption = opt_bool(NO_ENCRYPTION);
  static auto branch = db_branch();

  /* temporary table on 8.0 can't have key block size */
  if (!(branch.compare("8.0") == 0 && type == TEMPORARY)) {
    if (g_key_block_size.size() > 0)
      table->key_block_size =
          g_key_block_size[rand_int(g_key_block_size.size() - 1)];

    if (table->key_block_size > 0 && rand_int(2) == 0) {
      table->row_format = "COMPRESSED";
    }

    if (table->key_block_size == 0 && g_row_format.size() > 0)
      table->row_format = g_row_format[rand_int(g_row_format.size() - 1)];
  }

  /* with more number of tablespace there are more chances to have table in
   * tablespaces */
  static int tbs_count = opt_int(NUMBER_OF_GENERAL_TABLESPACE);

  /* temporary table can't have tablespace */
  if (table->type != TEMPORARY && g_tablespace.size() > 0 &&
      rand_int(tbs_count) != 0) {
    table->tablespace = g_tablespace[rand_int(g_tablespace.size() - 1)];

    if (table->tablespace.substr(table->tablespace.size() - 2, 2)
            .compare("_e") == 0)
      table->encryption = "Y";
    table->row_format.clear();
    if (g_innodb_page_size > INNODB_16K_PAGE_SIZE ||
        table->tablespace.compare("innodb_system") == 0 ||
        stoi(table->tablespace.substr(3, 2)) == g_innodb_page_size)
      table->key_block_size = 0;
    else
      table->key_block_size = std::stoi(table->tablespace.substr(3, 2));
  } else if (table->type != TEMPORARY && !no_encryption &&
             g_encryption.size() > 0) {
    table->encryption = g_encryption[rand_int(g_encryption.size() - 1)];
  }

  /* if temporary table encrypt variable set create encrypt table */
  static auto temp_table_encrypt =
      get_result("select @@innodb_temp_tablespace_encrypt", thd);

  if (strcmp(FORK, "Percona-Server") == 0 && db_branch().compare("5.7") == 0 &&
      temp_table_encrypt.compare("1") == 0 && table->type == TEMPORARY)
    table->encryption = 'y';

  /* if innodb system is encrypt , create encrypt table */
  static auto system_table_encrypt =
      get_result("select @@innodb_sys_tablespace_encrypt", thd);

  if (strcmp(FORK, "Percona-Server") == 0 && table->tablespace.size() > 0 &&
      table->tablespace.compare("innodb_system") == 0 &&
      system_table_encrypt.compare("1") == 0) {
    table->encryption = 'y';
  }

  /* 25 % tables are compress */
  if (table->type != TEMPORARY && table->tablespace.empty() and
      rand_int(3) == 1 && g_compression.size() > 0) {
    table->compression = g_compression[rand_int(g_compression.size() - 1)];
    table->row_format.clear();
    table->key_block_size = 0;
  }

  static auto engine = options->at(Option::ENGINE)->getString();
  table->engine = engine;

  table->CreateDefaultColumn();
  table->CreateDefaultIndex();

  return table;
}

/* prepare table definition */
std::string Table::definition() {
  std::string def = "CREATE";
  if (type == TABLE_TYPES::TEMPORARY)
    def += " TEMPORARY";
  def += " TABLE " + name_ + " (";

  if (columns_->size() == 0)
    throw std::runtime_error("no column in table " + name_);

  /* add columns */
  for (auto col : *columns_) {
    def += col->definition() + ", ";
  }

  /* if column has primary key */
  for (auto col : *columns_) {
    if (col->primary_key)
      def += " PRIMARY KEY(" + col->name_ + "), ";
  }

  if (indexes_->size() > 0) {
    for (auto id : *indexes_) {
      def += id->definition() + ", ";
    }
  }

  def.erase(def.length() - 2);

  def += " )";
  static auto no_encryption = opt_bool(NO_ENCRYPTION);

  if (!no_encryption && type != TEMPORARY)
    def += " ENCRYPTION='" + encryption + "'";

  if (!compression.empty())
    def += " COMPRESSION='" + compression + "'";

  if (!tablespace.empty())
    def += " TABLESPACE=" + tablespace;

  if (key_block_size > 1)
    def += " KEY_BLOCK_SIZE=" + std::to_string(key_block_size);

  if (row_format.size() > 0)
    def += " ROW_FORMAT=" + row_format;

  if (!engine.empty())
    def += " ENGINE=" + engine;

  return def;
}

/* create default table includes all tables*/
void create_default_tables(Thd1 *thd) {
  auto tables = opt_int(TABLES);

  auto only_temporary_tables = opt_bool(ONLY_TEMPORARY);

  if (!only_temporary_tables) {
    for (int i = 1; i <= tables; i++) {
      all_tables->push_back(Table::table_id(Table::NORMAL, i, thd));
      all_tables->push_back(Table::table_id(Table::PARTITION, i, thd));
    }
  }
}

/* return true if SQL is successful, else return false */
bool execute_sql(std::string sql, Thd1 *thd) {
  auto query = sql.c_str();
  static auto log_all = opt_bool(LOG_ALL_QUERIES);
  static auto log_failed = opt_bool(LOG_FAILED_QUERIES);
  static auto log_success = opt_bool(LOG_SUCCEDED_QUERIES);
  static auto log_query_duration = opt_bool(LOG_QUERY_DURATION);
  static auto log_client_output = opt_bool(LOG_CLIENT_OUTPUT);
  static auto log_query_numbers = opt_bool(LOG_QUERY_NUMBERS);
  std::chrono::system_clock::time_point begin, end;

  if (log_query_duration) {
    begin = std::chrono::system_clock::now();
  }

  auto res = mysql_real_query(thd->conn, query, strlen(query));

  if (log_query_duration) {
    end = std::chrono::system_clock::now();

    /* elpased time in micro-seconds */
    auto te_start = std::chrono::duration_cast<std::chrono::microseconds>(
        begin - start_time);
    auto te_query =
        std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    auto in_time_t = std::chrono::system_clock::to_time_t(begin);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%X");

    thd->thread_log << ss.str() << " " << te_start.count() << "=>"
                    << te_query.count() << "ms ";
  }
  thd->performed_queries_total++;

  if (res == 1) { // query failed
    thd->failed_queries_total++;
    thd->max_con_fail_count++;
    if (log_all || log_failed) {
      thd->thread_log << " F " << sql << std::endl;
      thd->thread_log << "Error " << mysql_error(thd->conn) << std::endl;
    }
    if (mysql_errno(thd->conn) == 2006) {
      thd->thread_log << "server gone, while processing " + sql;
      connection_lost = true;
    }
  } else {
    thd->max_con_fail_count = 0;
    thd->success = true;
    MYSQL_RES *result;
    result = mysql_store_result(thd->conn);

    /* log result */
    if (thd->store_result) {
      if (!result)
        throw std::runtime_error(sql + " does not return result set");
      auto row = mysql_fetch_row(result);
      thd->result = row[0];
    } else if (log_client_output) {
      if (result != NULL) {
        MYSQL_ROW row;
        unsigned int i, num_fields;

        num_fields = mysql_num_fields(result);
        while ((row = mysql_fetch_row(result))) {
          for (i = 0; i < num_fields; i++) {
            if (row[i]) {
              if (strlen(row[i]) == 0) {
                thd->client_log << "EMPTY"
                                << "#";
              } else {
                thd->client_log << row[i] << "#";
              }
            } else {
              thd->client_log << "#NO DATA"
                              << "#";
            }
          }
          if (log_query_numbers) {
            thd->client_log << ++thd->query_number;
          }
          thd->client_log << '\n';
        }
      }
    }

    /* log successful query */
    if (log_all || log_success) {
      thd->thread_log << " S " << sql;
      int number;
      if (result == NULL)
        number = mysql_affected_rows(thd->conn);
      else
        number = mysql_num_rows(result);
      thd->thread_log << " rows:" << number << std::endl;
      }
    mysql_free_result(result);
  }

  if (thd->ddl_query) {
    ddl_logs_write.lock();
    thd->ddl_logs << thd->thread_id << " " << sql << " "
                  << mysql_error(thd->conn) << std::endl;
    ddl_logs_write.unlock();
  }

  return (res == 0 ? 1 : 0);
}

/* load some records in table */
void load_default_data(Table *table, Thd1 *thd) {
  thd->ddl_query = false;
  static int initial_records =
      options->at(Option::INITIAL_RECORDS_IN_TABLE)->getInt();
  int rec = rand_int(initial_records);
  for (int i = 0; i < rec; i++) {
    table->InsertRandomRow(thd, false);
  }
}

void Table::SetEncryption(Thd1 *thd) {
  std::string sql = "ALTER TABLE " + name_ + " ENCRYPTION = '";
  std::string enc = g_encryption[rand_int(g_encryption.size() - 1)];
  sql += enc + "'";
  if (execute_sql(sql, thd)) {
    table_mutex.lock();
    encryption = enc;
    table_mutex.unlock();
  }
}

void Table::SetEncryptionInplace(Thd1 *thd) {
  std::string sql = "ALTER TABLESPACE `test/" + name_ + "` ENCRYPTION = '";
  std::string enc = g_encryption[rand_int(g_encryption.size() - 1)];
  sql += enc + "'";
  if (execute_sql(sql, thd)) {
    table_mutex.lock();
    encryption = enc;
    table_mutex.unlock();
  }
}

// todo pick relevant table //
void Table::SetTableCompression(Thd1 *thd) {
  std::string sql = "ALTER TABLE " + name_ + " COMPRESSION= '";
  std::string comp = g_compression[rand_int(g_compression.size() - 1)];
  sql += comp + "'";
  if (execute_sql(sql, thd)) {
    table_mutex.lock();
    compression = comp;
    table_mutex.unlock();
  }
}

// todo pick relevent table//
void Table::ModifyColumn(Thd1 *thd) {
  std::string sql = "ALTER TABLE " + name_ + " MODIFY COLUMN ";
  int i = 0;
  Column *col = nullptr;
  /* store old value */
  int length = 0;
  std::string default_value;
  bool auto_increment = false;
  bool compressed = false; // percona type compressed

  // try maximum 50 times to get a valid column
  while (i < 50 && col == nullptr) {
    auto col1 = columns_->at(rand_int(columns_->size() - 1));
    switch (col1->type_) {
    case Column::BLOB:
    case Column::GENERATED:
    case Column::VARCHAR:
    case Column::CHAR:
    case Column::FLOAT:
    case Column::DOUBLE:
    case Column::INT:
      col = col1;
      length = col->length;
      auto_increment = col->auto_increment;
      compressed = col->compressed;
      col->mutex.lock(); // lock column so no one can modify it //
      break;
      /* todo no support for BOOL INT so far */
    case Column::BOOL:
    case Column::COLUMN_MAX:
      break;
    }
    i++;
  }

  /* could not find a valid column to process */
  if (col == nullptr)
    return;

  col->length = rand_int(g_max_columns_length, 0);

  if (col->auto_increment == true and rand_int(5) == 0)
    col->auto_increment = false;

  if (col->compressed == true and rand_int(4) == 0)
    col->compressed = false;
  else if (options->at(Option::NO_COLUMN_COMPRESSION)->getBool() == false &&
           (col->type_ == Column::BLOB || col->type_ == Column::GENERATED ||
            col->type_ == Column::VARCHAR))
    col->compressed = true;

  sql += " " + col->definition() + pick_algorithm_lock();

  /* if not successful rollback */
  if (!execute_sql(sql, thd)) {
    col->length = length;
    col->auto_increment = auto_increment;
    col->compressed = compressed;
  }

  col->mutex.unlock();
}

/* alter table drop column */
void Table::DropColumn(Thd1 *thd) {
  table_mutex.lock();

  /* do not drop last column */
  if (columns_->size() == 1) {
    table_mutex.unlock();
    return;
  }
  auto ps = rand_int(columns_->size() - 1); // position

  auto name = columns_->at(ps)->name_;

  if (name.compare("pkey") == 0 || name.compare("pkey_rename") == 0) {
    table_mutex.unlock();
    return;
  }

  std::string sql = "ALTER TABLE " + name_ + " DROP COLUMN " + name;

  sql += pick_algorithm_lock();
  table_mutex.unlock();

  if (execute_sql(sql, thd)) {
    table_mutex.lock();

    std::vector<int> indexes_to_drop;
    for (auto id = indexes_->begin(); id != indexes_->end(); id++) {
      auto index = *id;

      for (auto id_col = index->columns_->begin();
           id_col != index->columns_->end(); id_col++) {
        auto ic = *id_col;
        if (ic->column->name_.compare(name) == 0) {
          if (index->columns_->size() == 1) {
            delete index;
            indexes_to_drop.push_back(id - indexes_->begin());
          } else {
            delete ic;
            index->columns_->erase(id_col);
          }
          break;
        }
      }
    }
    std::sort(indexes_to_drop.begin(), indexes_to_drop.end(),
              std::greater<int>());

    for (auto &i : indexes_to_drop) {
      indexes_->at(i) = indexes_->back();
      indexes_->pop_back();
    }
    // table->indexes_->erase(id);

    for (auto pos = columns_->begin(); pos != columns_->end(); pos++) {
      auto col = *pos;
      if (col->name_.compare(name) == 0) {
        col->mutex.lock();
        delete col;
        columns_->erase(pos);
        break;
      }
    }
    table_mutex.unlock();
  }
}

/* alter table add random column */
void Table::AddColumn(Thd1 *thd) {

  static auto no_use_virtual = opt_bool(NO_VIRTUAL_COLUMNS);
  static auto use_blob = !options->at(Option::NO_BLOB)->getBool();

  std::string sql = "ALTER TABLE " + name_ + " ADD COLUMN ";

  Column::COLUMN_TYPES col_type = Column::COLUMN_MAX;

  auto use_virtual = true;

  // lock table to create definition
  table_mutex.lock();

  if (no_use_virtual ||
      (columns_->size() == 1 && columns_->at(0)->auto_increment == true))
    use_virtual = false;

  while (col_type == Column::COLUMN_MAX) {
    /* new columns are in ratio of 3:2:1:1:1:1
     * INT:VARCHAR:CHAR:BLOB:BOOL:GENERATD */
    auto prob = rand_int(8);
    if (prob < 3)
      col_type = Column::INT;
    else if (prob < 5)
      col_type = Column::VARCHAR;
    else if (prob < 6)
      col_type = Column::CHAR;
    else if (prob < 7 && use_virtual)
      col_type = Column::GENERATED;
    else if (prob < 8)
      col_type = Column::BOOL;
    else if (prob < 9 && use_blob)
      col_type = Column::BLOB;
  }

  Column *tc;

  std::string name = "N" + std::to_string(rand_int(300));

  if (col_type == Column::GENERATED)
    tc = new Generated_Column(name, this);
  else if (col_type == Column::BLOB)
    tc = new Blob_Column(name, this);
  else
    tc = new Column(name, this, col_type);

  sql += " " + tc->definition();
  sql += pick_algorithm_lock();

  table_mutex.unlock();

  if (execute_sql(sql, thd)) {
    table_mutex.lock();
    auto add_new_column =
        true; // check if there is already a column with this name
    for (auto col : *columns_) {
      if (col->name_.compare(tc->name_) == 0)
        add_new_column = false;
    }

    if (add_new_column)
      AddInternalColumn(tc);
    else
      delete tc;

    table_mutex.unlock();
  } else
    delete tc;
}

/* randomly drop some index of table */
void Table::DropIndex(Thd1 *thd) {
  table_mutex.lock();
  if (indexes_ != nullptr && indexes_->size() > 0) {
    auto index = indexes_->at(rand_int(indexes_->size() - 1));
    auto name = index->name_;
    std::string sql = "ALTER TABLE " + name_ + " DROP INDEX " + name;
    sql += pick_algorithm_lock();
    table_mutex.unlock();
    if (execute_sql(sql, thd)) {
      table_mutex.lock();
      for (size_t i = 0; i < indexes_->size(); i++) {
        auto ix = indexes_->at(i);
        if (ix->name_.compare(name) == 0) {
          delete ix;
          indexes_->at(i) = indexes_->back();
          indexes_->pop_back();
          break;
        }
      }
      table_mutex.unlock();
    }
  } else {
    table_mutex.unlock();
    thd->thread_log << "no index to drop " + name_ << std::endl;
  }
}

/*randomly add some index on the table */
void Table::AddIndex(Thd1 *thd) {
  auto i = rand_int(1000);
  Index *id = new Index(name_ + std::to_string(i));

  static size_t max_columns = opt_int(INDEX_COLUMNS);
  table_mutex.lock();

  /* number of columns to be added */
  int no_of_columns = rand_int(
      (max_columns < columns_->size() ? max_columns : columns_->size()), 1);

  std::vector<int> col_pos; // position of columns

  /* pick some columns */
  while (col_pos.size() < (size_t)no_of_columns) {
    int current = rand_int(columns_->size() - 1);
    /* auto-inc column should be first column in auto_inc_index */
    bool already_added = false;
    for (auto id : col_pos) {
      if (id == current)
        already_added = true;
    }
    if (!already_added)
      col_pos.push_back(current);
  } // while

  for (auto pos : col_pos) {
    auto col = columns_->at(pos);
    static bool no_desc_support = opt_bool(NO_DESC_INDEX);
    bool column_desc = false;
    if (!no_desc_support) {
      column_desc = rand_int(100) < DESC_INDEXES_IN_COLUMN
                        ? true
                        : false; // 33 % are desc //
    }
    id->AddInternalColumn(new Ind_col(col, column_desc)); // desc is set as true
  }

  std::string sql = "ALTER TABLE " + name_ + " ADD " + id->definition();
  sql += pick_algorithm_lock();
  table_mutex.unlock();

  if (execute_sql(sql, thd)) {
    table_mutex.lock();
    auto do_not_add = false; // check if there is already a index with this name
    for (auto ind : *indexes_) {
      if (ind->name_.compare(id->name_) == 0)
        do_not_add = true;
    }
    if (!do_not_add)
      AddInternalIndex(id);
    else
      delete id;

    table_mutex.unlock();
  } else {
    delete id;
  }
}

void Table::DeleteAllRows(Thd1 *thd) {
  std::string sql = "DELETE FROM " + name_;
  execute_sql(sql, thd);
}

void Table::SelectAllRow(Thd1 *thd) {
  std::string sql = "SELECT * FROM " + name_;
  execute_sql(sql, thd);
}

void Table::ColumnRename(Thd1 *thd) {
  table_mutex.lock();
  auto ps = rand_int(columns_->size() - 1);
  auto name = columns_->at(ps)->name_;
  /* ALTER column to _rename or back to orignal_name */
  std::string new_name = "_rename";
  static auto s = new_name.size();
  if (name.size() > s && name.substr(name.length() - s).compare("_rename") == 0)
    new_name = name.substr(0, name.length() - s);
  else
    new_name = name + new_name;
  std::string sql =
      "ALTER TABLE " + name_ + " RENAME COLUMN " + name + " To " + new_name;
  table_mutex.unlock();
  if (execute_sql(sql, thd)) {
    table_mutex.lock();
    for (auto &col : *columns_) {
      if (col->name_.compare(name) == 0)
        col->name_ = new_name;
    }
    table_mutex.unlock();
  }
}

void Table::DeleteRandomRow(Thd1 *thd) {
  table_mutex.lock();
  auto where = -1;
  auto prob = rand_int(98);
  bool only_bool = true;
  int pk_pos = -1;

  for (size_t i = 0; i < columns_->size(); i++) {
    auto col = columns_->at(i);
    if (col->type_ != Column::BOOL)
      only_bool = false;
    if (col->primary_key)
      pk_pos = i;
  }

  /* 50% time we use primary key column */
  if (pk_pos != -1 && rand_int(100) > 50)
    where = pk_pos;
  else {
    /* iterate over and over to find a valid column */
    while (where < 0) {
      auto col_pos = rand_int(columns_->size() - 1);
      switch (columns_->at(col_pos)->type_) {
      case Column::BOOL:
        if (only_bool || rand_int(1000) == 0)
          where = col_pos;
        break;
      case Column::INT:
      case Column::FLOAT:
      case Column::DOUBLE:
      case Column::VARCHAR:
      case Column::CHAR:
      case Column::BLOB:
      case Column::GENERATED:
        where = col_pos;
        break;
      case Column::COLUMN_MAX:
        break;
      }
    }
  }
  std::string sql = "DELETE FROM " + name_ + " WHERE " +
                   columns_->at(where)->name_;
  if (prob <= 90)
    sql += " = " + columns_->at(where)->rand_value();
  else if (prob <= 92)
    sql += " >= " +
          columns_->at(where)->rand_value() + " AND " +
          columns_->at(where)->name_ + " <= " +
          columns_->at(where)->rand_value();
  else if (prob <= 94)
    sql += " IN (" +
          columns_->at(where)->rand_value() + "," +
          columns_->at(where)->rand_value() + ")";
  else if (prob <= 96)
    sql += " BETWEEN " +
          columns_->at(where)->rand_value() + " AND " +
          columns_->at(where)->rand_value();
  else
    sql += " LIKE '%" +
          columns_->at(where)->rand_value() + "%'";

  table_mutex.unlock();
  execute_sql(sql, thd);
}

void Table::SelectRandomRow(Thd1 *thd) {
  table_mutex.lock();
  auto where = rand_int(columns_->size() - 1);
  auto prob = rand_int(100);
  std::string sql = "SELECT * FROM " + name_ + " WHERE " +
                   columns_->at(where)->name_;
  if (rand_int(1000) < 2)
    sql += " NOT BETWEEN " +
          columns_->at(where)->rand_value() + " AND " +
          columns_->at(where)->rand_value();
  else if (prob <= 90)
    sql += " = " +
          columns_->at(where)->rand_value();
  else if (prob <= 92)
    sql += " >= " +
          columns_->at(where)->rand_value();
  else if (prob <= 94)
    sql += " >= " +
          columns_->at(where)->rand_value() + " AND " +
          columns_->at(where)->name_ + " <= " +
          columns_->at(where)->rand_value();
  else if (prob <= 96)
    sql += " IN (" +
          columns_->at(where)->rand_value() + ", " +
          columns_->at(where)->rand_value() + ")";
  else if (prob <= 98)
    sql += " LIKE '%" +
          columns_->at(where)->rand_value() + "%'";
  else
    sql += " BETWEEN " +
          columns_->at(where)->rand_value() + " AND " +
          columns_->at(where)->rand_value();

  table_mutex.unlock();
  execute_sql(sql, thd);
}

/* update random row */
void Table::UpdateRandomROW(Thd1 *thd) {
  table_mutex.lock();
  auto set = rand_int(columns_->size() - 1);
  auto where = rand_int(columns_->size() - 1);
  auto prob = rand_int(98);
  std::string sql = "UPDATE " + name_ + " SET " +
                   columns_->at(set)->name_ + " = " +
                   columns_->at(set)->rand_value() + " WHERE ";

  /* if tables has pkey try to use that in where clause for 50% cases */
  for (size_t i = 0; i < columns_->size(); i++) {
    if (columns_->at(i)->primary_key && rand_int(100) <= 50) {
      where = i;
      break;
    }
  }
  if (prob <= 90)
    sql += columns_->at(where)->name_ + " = " +
          columns_->at(where)->rand_value();
  else if (prob <= 92)
    sql += columns_->at(where)->name_ + " >= " +
          columns_->at(where)->rand_value() + " AND " + " <= " +
          columns_->at(where)->rand_value();
  else if (prob <= 94)
    sql += columns_->at(where)->name_ + " IN (" +
          columns_->at(where)->rand_value() + "," +
          columns_->at(where)->rand_value() + ")";
  else if (prob <= 96)
    sql += columns_->at(where)->name_ + " BETWEEN " +
          columns_->at(where)->rand_value() + " AND " +
          columns_->at(where)->rand_value();
  else
    sql += columns_->at(where)->name_ + " LIKE '%" +
          columns_->at(where)->rand_value() + "%'";

  table_mutex.unlock();
  execute_sql(sql, thd);
}

void Table::InsertRandomRow(Thd1 *thd, bool is_lock) {
  if (is_lock)
    table_mutex.lock();
  std::string vals = "";
  std::string type = "INSERT";

  if (is_lock)
    type = rand_int(3) == 0 ? "INSERT" : "REPLACE";

  std::string sql = type + " INTO " + name_ + "  ( ";
  for (auto &column : *columns_) {
    sql += column->name_ + " ,";
    auto val = column->rand_value();
    if (column->auto_increment == true && rand_int(100) < 10)
      val = "NULL";
    vals += " " + val + ",";
  }

  if (vals.size() > 0) {
    vals.pop_back();
    sql.pop_back();
  }
  sql += ") VALUES(" + vals;
  sql += " )";
  if (is_lock)
    table_mutex.unlock();
  execute_sql(sql, thd);
}

/* set mysqld_variable */
void set_mysqld_variable(Thd1 *thd) {
  static int total_probablity = sum_of_all_server_options();
  int rd = rand_int(total_probablity);
  for (auto &opt : *server_options) {
    if (rd <= opt->prob) {
      std::string sql = "SET ";
      sql += rand_int(3) == 0 ? " SESSION " : " GLOBAL ";
      sql += opt->name + "=" + opt->values.at(rand_int(opt->values.size() - 1));
      execute_sql(sql, thd);
    }
  }
}

/* alter tablespace set encryption */
void alter_tablespace_encryption(Thd1 *thd) {
  if (g_tablespace.size() > 0) {
    std::string sql = "ALTER TABLESPACE " +
                      g_tablespace[rand_int(g_tablespace.size() - 1)] +
                      " ENCRYPTION ";
    sql += (rand_int(1) == 0 ? "'Y'" : "'N'");
    execute_sql(sql, thd);
  }
}

/* alter database set encryption */
void alter_database_encryption(Thd1 *thd) {
  std::string sql = "ALTER DATABASE test ENCRYPTION ";
  sql += (rand_int(1) == 0 ? "'Y'" : "'N'");
  execute_sql(sql, thd);
}

/* create,alter,drop undo tablespace */
static void create_alter_drop_undo(Thd1 *thd) {
  auto x = rand_int(100);
  if (x < 20) {
    std::string name =
        g_undo_tablespace[rand_int(g_undo_tablespace.size() - 1)];
    std::string sql =
        "CREATE UNDO TABLESPACE " + name + " ADD DATAFILE '" + name + ".ibu'";
    execute_sql(sql, thd);
  }
  if (x < 40) {
    std::string sql = "DROP UNDO TABLESPACE " +
                      g_undo_tablespace[rand_int(g_undo_tablespace.size() - 1)];
    execute_sql(sql, thd);
  } else {
    std::string sql =
        "ALTER UNDO TABLESPACE " +
        g_undo_tablespace[rand_int(g_undo_tablespace.size() - 1)] + " SET ";
    sql += (rand_int(1) == 0 ? "ACTIVE" : "INACTIVE");
    execute_sql(sql, thd);
  }
}

/* alter tablespace rename */
void alter_tablespace_rename(Thd1 *thd) {
  if (g_tablespace.size() > 0) {
    auto tablespace = g_tablespace[rand_int(g_tablespace.size() - 1),
                                   1]; // don't pick innodb_system;
    std::string sql = "ALTER TABLESPACE " + tablespace;
    if (rand_int(1) == 0)
      sql += "_rename RENAME TO " + tablespace;
    else
      sql += " RENAME TO " + tablespace + "_rename";
    execute_sql(sql, thd);
  }
}

/* load special sql from a file */
static std::vector<std::string> load_special_sql_from() {
  std::vector<std::string> array;
  auto file = opt_string(SQL_FILE);
  std::string line;

  std::ifstream myfile(file);
  if (myfile.is_open()) {
    while (!myfile.eof()) {
      getline(myfile, line);
      array.push_back(line);
    }
    myfile.close();
  } else
    throw std::runtime_error("unable to open file " + file);
  return array;
}

/* return preformatted sql */
static void special_sql(std::vector<Table *> *all_tables, Thd1 *thd) {

  static std::vector<std::string> all_sql = load_special_sql_from();
  enum sql_col_types { INT, VARCHAR };

  if (all_sql.size() == 0)
    return;

  struct table {
    table(std::string n, std::vector<std::string> i, std::vector<std::string> v)
        : name(n), int_col(i), varchar_col(v){};
    std::string name;
    std::vector<std::string> int_col;
    std::vector<std::string> varchar_col;
  };

  auto sql = all_sql[rand_int(all_sql.size() - 1)];

  /* parse SQL in table */
  std::vector<std::vector<int>> sql_tables;

  int tab_sql = 1; // number of tables in sql
  bool table_found;

  do { // search for table
    std::smatch match;
    std::string tab_p = "T" + std::to_string(tab_sql); // table pattern

    if (regex_search(sql, match, std::regex(tab_p))) {
      table_found = true;
      sql_tables.push_back({0, 0});

      int col_sql = 1;
      bool column_found;

      do { // search of int column
        std::string col_p = tab_p + "_INT_" + std::to_string(col_sql);
        if (regex_search(sql, match, std::regex(col_p))) {
          column_found = true;
          sql_tables.at(tab_sql - 1).at(INT)++;
          col_sql++;
        } else
          column_found = false;
      } while (column_found);

      col_sql = 1;
      do {
        std::string col_p = tab_p + "_VARCHAR_" + std::to_string(col_sql);
        if (regex_search(sql, match, std::regex(col_p))) {
          column_found = true;
          sql_tables.at(tab_sql - 1).at(VARCHAR)++;
          col_sql++;
        } else
          column_found = false;
      } while (column_found);
    } else
      table_found = false;
    tab_sql++;
  } while (table_found);

  std::vector<table> final_tables;

  /* try at max 100 times */
  int table_check = 100;

  while (sql_tables.size() > 0 && table_check-- > 0) {

    auto int_columns = sql_tables.back().at(INT);
    auto varchar_columns = sql_tables.back().at(VARCHAR);
    std::vector<std::string> int_cols_str, var_cols_str;
    int column_check = 20;
    auto table = all_tables->at(rand_int(all_tables->size() - 1));
    table->table_mutex.lock();
    auto columns = table->columns_;

    // find columns in table //
    do {
      auto col = columns->at(rand_int(columns->size() - 1));

      if (int_columns > 0 && col->type_ == Column::INT) {
        int_cols_str.push_back(col->name_);
        int_columns--;
      }
      if (varchar_columns > 0 && col->type_ == Column::VARCHAR) {
        var_cols_str.push_back(col->name_);
        varchar_columns--;
      }

      if (int_columns == 0 && varchar_columns == 0) {
        final_tables.emplace_back(table->name_, int_cols_str, var_cols_str);
        sql_tables.pop_back();
      }
    } while (!(int_columns == 0 && varchar_columns == 0) && column_check-- > 0);

    table->table_mutex.unlock();
  }

  if (sql_tables.size() == 0) {

    for (size_t i = 0; i < final_tables.size(); i++) {
      auto table = final_tables.at(i);
      auto table_name = "T" + std::to_string(i + 1);

      /* replace int column */
      for (size_t j = 0; j < table.int_col.size(); j++)
        sql = std::regex_replace(
            sql, std::regex(table_name + "_INT_" + std::to_string(j + 1)),
            table_name + "." + table.int_col.at(j));

      /* replace varchar column */
      for (size_t j = 0; j < table.varchar_col.size(); j++)
        sql = std::regex_replace(
            sql, std::regex(table_name + "_VARCHAR_" + std::to_string(j + 1)),
            table_name + "." + table.varchar_col.at(j));

      /* replace table "T1 " => tt_N T1 */
      sql = std::regex_replace(sql, std::regex(table_name + " "),
                               table.name + " " + table_name + " ");
      /* replace table "T1$" => tt_N T1*/
      sql = std::regex_replace(sql, std::regex(table_name + "$"),
                               table.name + " " + table_name + "");
    }

    execute_sql(sql, thd);
  } else
    std::cout << "NOT ABLE TO FIND any SQL" << std::endl;
}

/* save metadata to a file */
void save_metadata_to_file() {
  std::string path = opt_string(METADATA_PATH);
  if (path.size() == 0)
    path = opt_string(LOGDIR);
  auto file =
      path + "/step_" + std::to_string(options->at(Option::STEP)->getInt()) + ".dll";
  std::cout << "Saving metadata to file " << file << std::endl;

  StringBuffer sb;
  PrettyWriter<StringBuffer> writer(sb);
  writer.StartObject();
  writer.String("version");
  writer.Uint(version);
  writer.String(("tables"));
  writer.StartArray();
  for (auto j = all_tables->begin(); j != all_tables->end(); j++) {
    auto table = *j;
    table->Serialize(writer);
  }
  writer.EndArray();
  writer.EndObject();
  std::ofstream of(file);
  of << sb.GetString();

  if (!of.good())
    throw std::runtime_error("can't write the JSON string to the file!");
}

/* create in memory data about tablespaces, row_format, key_block size and undo
 * tablespaces */
void create_in_memory_data() {

  /* Adjust the tablespaces */
  if (!options->at(Option::NO_TABLESPACE)->getBool()) {
    g_tablespace = {"tab02k", "tab04k"};
    g_tablespace.push_back("innodb_system");
    if (g_innodb_page_size >= INNODB_8K_PAGE_SIZE) {
      g_tablespace.push_back("tab08k");
    }
    if (g_innodb_page_size >= INNODB_16K_PAGE_SIZE) {
      g_tablespace.push_back("tab16k");
    }
    if (g_innodb_page_size >= INNODB_32K_PAGE_SIZE) {
      g_tablespace.push_back("tab32k");
    }
    if (g_innodb_page_size >= INNODB_64K_PAGE_SIZE) {
      g_tablespace.push_back("tab64k");
    }

    /* add addtional tablespace */
    auto tbs_count = opt_int(NUMBER_OF_GENERAL_TABLESPACE);
    if (tbs_count > 1) {
      auto current_size = g_tablespace.size();
      for (size_t i = 0; i < current_size; i++) {
        for (int j = 1; j <= tbs_count; j++)
          if (g_tablespace[i].compare("innodb_system") == 0)
            continue;
          else
            g_tablespace.push_back(g_tablespace[i] + std::to_string(j));
      }
    }
  }

  /* set some of tablespace encrypt */
  if (!options->at(Option::NO_ENCRYPTION)->getBool() &&
      !(strcmp(FORK, "MySQL") == 0 && db_branch().compare("5.7") == 0)) {
    int i = 0;
    for (auto &tablespace : g_tablespace) {
      if (i++ % 2 == 0 &&
          tablespace.compare("innodb_system") != 0) // alternate tbs are encrypt
        tablespace += "_e";
    }
  }

  std::string row_format = opt_string(ROW_FORMAT);
  if (row_format.compare("uncompressed") == 0) {
    g_row_format = {"DYNAMIC", "REDUNDANT"};
  } else if (row_format.compare("all") == 0) {
    g_row_format = {"DYNAMIC", "REDUNDANT", "COMPRESSED"};
    g_key_block_size = {0, 0, 1, 2, 4};
  } else if (row_format.compare("none") == 0) {
    g_key_block_size.empty();
  } else {
    g_row_format.push_back(row_format);
  }

  if (g_innodb_page_size > INNODB_16K_PAGE_SIZE) {
    g_row_format.clear();
    g_key_block_size.clear();
  }

  int undo_tbs_count = opt_int(NUMBER_OF_UNDO_TABLESPACE);
  if (undo_tbs_count > 0) {
    for (int i = 1; i <= undo_tbs_count; i++) {
      g_undo_tablespace.push_back("undo_00" + std::to_string(i));
    }
  }
}

/*load objects from a file */
static std::string load_metadata_from_file() {
  auto previous_step = options->at(Option::STEP)->getInt() - 1;
  auto path = opt_string(METADATA_PATH);
  if (path.size() == 0)
    path = opt_string(LOGDIR);
  auto file = path + "/step_" + std::to_string(previous_step) + ".dll";
  FILE *fp = fopen(file.c_str(), "r");

  if (fp == nullptr)
    throw std::runtime_error("unable to open file " + file);

  char readBuffer[65536];
  FileReadStream is(fp, readBuffer, sizeof(readBuffer));
  Document d;
  d.ParseStream(is);
  auto v = d["version"].GetInt();

  if (d["version"].GetInt() != version)
    throw std::runtime_error("version mismatch between " + file +
                             " and codebase " + " file::version is " +
                             std::to_string(v) + " code::version is " +
                             std::to_string(version));

  for (auto &tab : d["tables"].GetArray()) {
    Table *table;
    std::string name = tab["name"].GetString();

    if (std::count(name.begin(), name.end(), '_') > 1) {
      std::string table_type = name.substr(name.size() - 2, 2);
      if (table_type.compare(partition_string) == 0)
        table = new Partition_table(name);
      else
        throw std::runtime_error("unhandled table type");
    } else
      table = new Table(name);

    std::string engine = tab["engine"].GetString();
    if (engine.compare("default") != 0) {
      table->engine = engine;
    }

    std::string row_format = tab["row_format"].GetString();
    if (row_format.compare("default") != 0) {
      table->row_format = row_format;
    }

    std::string tablespace = tab["tablespace"].GetString();
    if (tablespace.compare("file_per_table") != 0) {
      table->tablespace = tablespace;
    }

    table->encryption = tab["encryption"].GetString();
    table->compression = tab["compression"].GetString();

    table->key_block_size = tab["key_block_size"].GetInt();

    /* save columns */
    for (auto &col : tab["columns"].GetArray()) {
      Column *a;
      std::string type = col["type"].GetString();

      if (type.compare("INT") == 0 || type.compare("CHAR") == 0 ||
          type.compare("VARCHAR") == 0 || type.compare("BOOL") == 0 ||
          type.compare("FLOAT") == 0 || type.compare("DOUBLE") == 0) {
        a = new Column(col["name"].GetString(), type, table);
      } else if (type.compare("GENERATED") == 0) {
        auto name = col["name"].GetString();
        auto clause = col["clause"].GetString();
        auto sub_type = col["sub_type"].GetString();
        a = new Generated_Column(name, table, clause, sub_type);
      } else if (type.compare("BLOB") == 0) {
        auto sub_type = col["sub_type"].GetString();
        a = new Blob_Column(col["name"].GetString(), table, sub_type);
      } else
        throw std::runtime_error("unhandled column type");

      a->null = col["null"].GetBool();
      a->auto_increment = col["auto_increment"].GetBool();
      a->length = col["lenght"].GetInt(),
      a->primary_key = col["primary_key"].GetBool();
      a->compressed = col["compressed"].GetBool();
      table->AddInternalColumn(a);
    }

    for (auto &ind : tab["indexes"].GetArray()) {
      Index *index = new Index(ind["name"].GetString());

      for (auto &ind_col : ind["index_columns"].GetArray()) {
        std::string index_base_column = ind_col["name"].GetString();

        for (auto &column : *table->columns_) {
          if (index_base_column.compare(column->name_) == 0) {
            index->AddInternalColumn(
                new Ind_col(column, ind_col["desc"].GetBool()));
            break;
          }
        }
      }
      table->AddInternalIndex(index);
    }

    all_tables->push_back(table);
    options->at(Option::TABLES)->setInt(all_tables->size());
  }
  fclose(fp);
  return file;
}

/* clean tables from memory,random_strs */
void clean_up_at_end() {
  for (auto &table : *all_tables)
    delete table;
  delete all_tables;
  delete random_strs;
}

/* create new database and tablespace */
void create_database_tablespace(Thd1 *thd) {

  /* drop database test*/
  execute_sql("DROP DATABASE test", thd);
  execute_sql("CREATE DATABASE test", thd); // todo encrypt database/schema

  for (auto &tab : g_tablespace) {
    if (tab.compare("innodb_system") == 0)
      continue;

    std::string sql =
        "CREATE TABLESPACE " + tab + " ADD DATAFILE '" + tab + ".ibd' ";

    if (g_innodb_page_size <= INNODB_16K_PAGE_SIZE) {
      sql += " FILE_BLOCK_SIZE " + tab.substr(3, 3);
    }

    /* encrypt tablespace */
    if (!options->at(Option::NO_ENCRYPTION)->getBool()) {
      if (tab.substr(tab.size() - 2, 2).compare("_e") == 0)
        sql += " ENCRYPTION='Y'";
      else if (db_branch().compare("5.7") != 0)
        sql += " ENCRYPTION='N'";
    }

    /* firest try to rename tablespace back */
    if (db_branch().compare("5.7") != 0)
      execute_sql("ALTER TABLESPACE " + tab + "_rename rename to " + tab, thd);

    execute_sql("DROP TABLESPACE " + tab, thd);

    if (!execute_sql(sql, thd))
      throw std::runtime_error("error in " + sql);
  }

  if (db_branch().compare("5.7") != 0) {
    for (auto &name : g_undo_tablespace) {
      std::string sql =
          "CREATE UNDO TABLESPACE " + name + " ADD DATAFILE '" + name + ".ibu'";
      execute_sql(sql, thd);
    }
  }
}

/* load metadata */
bool Thd1::load_metadata() {
  sum_of_all_opts = sum_of_all_options(this);

  auto seed = opt_int(INITIAL_SEED);
  seed += options->at(Option::STEP)->getInt();
  random_strs = random_strs_generator(seed);

  /*set seed for current thread*/
  auto initial_seed = opt_int(INITIAL_SEED);
  rng = std::mt19937(initial_seed);

  /* find out innodb page_size */
  if (options->at(Option::ENGINE)->getString().compare("INNODB") == 0)
    g_innodb_page_size =
        std::stoi(get_result("select @@innodb_page_size", this)) / 1024;

  /* create in-memory data for general tablespaces */
  create_in_memory_data();

  if (options->at(Option::STEP)->getInt() > 1) {
    auto file = load_metadata_from_file();
    std::cout << "metadata loaded from " << file << std::endl;
  } else {
    create_database_tablespace(this);
    create_default_tables(this);
    std::cout << "metadata created randomly" << std::endl;
  }

  if (options->at(Option::TABLES)->getInt() <= 0)
    throw std::runtime_error("no table to work on \n");

  return 1;
}

void Thd1::run_some_query() {
  bool just_ddl = opt_bool(JUST_LOAD_DDL);
  execute_sql("USE " + options->at(Option::DATABASE)->getString(), this);

  /* first create temporary tables metadata if requried */
  int temp_tables;
  if (options->at(Option::ONLY_TEMPORARY)->getBool())
    temp_tables = options->at(Option::TABLES)->getInt();
  else if (options->at(Option::NO_TEMPORARY)->getBool())
    temp_tables = 0;
  else
    temp_tables = options->at(Option::TABLES)->getInt() /
                  options->at(Option::TEMPORARY_TO_NORMAL_RATIO)->getInt();

  /* create temporary table */
  std::vector<Table *> *all_session_tables = new std::vector<Table *>;
  for (int i = 0; i < temp_tables; i++) {

    ddl_query = true;
    Table *table = Table::table_id(Table::TEMPORARY, i, this);
    if (!execute_sql(table->definition(), this))
      throw std::runtime_error("Create table failed " + table->name_);
    all_session_tables->push_back(table);

    /* load default data in temporary table */
    if (!just_ddl) {
      load_default_data(table, this);
    }
  }

  /* if step is 1 then , create all tables */
  if (options->at(Option::STEP)->getInt() == 1) {
    auto current = table_started++;
    while (current < all_tables->size()) {
      auto table = all_tables->at(current);
      ddl_query = true;
      if (!execute_sql(table->definition(), this))
        throw std::runtime_error("Create table failed " + table->name_);
      /* load default data in table*/
      if (!just_ddl) {
        load_default_data(table, this);
      }
      table_completed++;
      current = table_started++;
    }

    // wait for all tables to finish loading
    while (table_completed < all_tables->size()) {
      thread_log << "Waiting for all threds to finish initial load "
                 << std::endl;
      std::chrono::seconds dura(1);
      std::this_thread::sleep_for(dura);
    }
  }

  if (just_ddl)
    return;

  if (!lock_stream.test_and_set())
    std::cout << "starting random load in "
              << options->at(Option::THREADS)->getInt() << " threads."
              << std::endl;

  auto sec = opt_int(NUMBER_OF_SECONDS_WORKLOAD);
  auto begin = std::chrono::system_clock::now();
  auto end =
      std::chrono::system_clock::time_point(begin + std::chrono::seconds(sec));

  /* set seed for current thread */
  rng = std::mt19937(set_seed(this));
  thread_log << thread_id << " value of rand_int(100) " << rand_int(100)
             << std::endl;

  /* combine session tables with all tables */
  all_session_tables->insert(all_session_tables->end(), all_tables->begin(),
                             all_tables->end());

  /* freqency of all options per thread */
  int opt_feq[Option::MAX][2] = {{0, 0}};

  /* variables related to trxs */
  static auto trx_prob = options->at(Option::TRANSATION_PRB_K)->getInt();
  static auto trx_max_size = options->at(Option::TRANSACTIONS_SIZE)->getInt();
  static auto commit_to_rollback =
      options->at(Option::COMMMIT_TO_ROLLBACK_RATIO)->getInt();
  static auto savepoint_prob = options->at(Option::SAVEPOINT_PRB_K)->getInt();

  int trx_left = -1; // -1 for single trx
  int current_save_point = 0;
  while (std::chrono::system_clock::now() < end) {

    /* check if we need to make sql as part of existing or new trx */
    if (trx_prob > 0) {
      if (trx_left == 0)
        execute_sql((rand_int(commit_to_rollback) == 0 ? "ROLLBACK" : "COMMIT"),
                    this);

      /*  check if sql are chossen as part of statement or single trx */
      if (trx_left <= 0 && trx_max_size > 0 && rand_int(1000) < trx_prob) {
        current_save_point = 0;
        if (rand_int(1000) == 1)
          trx_left = trx_max_size * 100;
        else
          trx_left = rand_int(trx_max_size);
        execute_sql("START TRANSACTION", this);
      }

      /* use savepoint or rollback to savepoint */
      if (trx_left > 0 && savepoint_prob > 0) {
        if (rand_int(1000) < savepoint_prob)
          execute_sql("SAVEPOINT SAVE" + std::to_string(++current_save_point), this);

        /* 1/4 chances of rollbacking to savepoint */
        if (current_save_point > 0 && rand_int(1000 * 4) < savepoint_prob) {
          auto sv = rand_int(current_save_point, 1);
          execute_sql("ROLLBACK TO SAVEPOINT SAVE" + std::to_string(sv), this);
          current_save_point = sv - 1;
        }

        trx_left--;
      }
    }

    auto table =
        all_session_tables->at(rand_int(all_session_tables->size() - 1));
    auto option = pick_some_option();
    ddl_query = options->at(option)->ddl == true ? true : false;

    switch (option) {
    case Option::DROP_INDEX:
      table->DropIndex(this);
      break;
    case Option::ADD_INDEX:
      table->AddIndex(this);
      break;
    case Option::DROP_COLUMN:
      table->DropColumn(this);
      break;
    case Option::ADD_COLUMN:
      table->AddColumn(this);
      break;
    case Option::TRUNCATE:
      table->Truncate(this);
      break;
    case Option::DROP_CREATE:
      table->DropCreate(this);
      break;
    case Option::ALTER_TABLE_ENCRYPTION:
      table->SetEncryption(this);
      break;
    case Option::ALTER_TABLE_ENCRYPTION_INPLACE:
      table->SetEncryptionInplace(this);
      break;
    case Option::ALTER_TABLE_COMPRESSION:
      table->SetTableCompression(this);
      break;
    case Option::ALTER_COLUMN_MODIFY:
      table->ModifyColumn(this);
      break;
    case Option::SET_GLOBAL_VARIABLE:
      set_mysqld_variable(this);
      break;
    case Option::ALTER_TABLESPACE_ENCRYPTION:
      alter_tablespace_encryption(this);
      break;
    case Option::ALTER_TABLESPACE_RENAME:
      alter_tablespace_rename(this);
      break;
    case Option::SELECT_ALL_ROW:
      table->SelectAllRow(this);
      break;
    case Option::SELECT_ROW_USING_PKEY:
      table->SelectRandomRow(this);
      break;
    case Option::INSERT_RANDOM_ROW:
      table->InsertRandomRow(this, true);
      break;
    case Option::DELETE_ALL_ROW:
      table->DeleteAllRows(this);
      break;
    case Option::DELETE_ROW_USING_PKEY:
      table->DeleteRandomRow(this);
      break;
    case Option::UPDATE_ROW_USING_PKEY:
      table->UpdateRandomROW(this);
      break;
    case Option::OPTIMIZE:
      table->Optimize(this);
      break;
    case Option::ANALYZE:
      table->Analyze(this);
      break;
    case Option::RENAME_COLUMN:
      table->ColumnRename(this);
      break;
    case Option::ALTER_MASTER_KEY:
      execute_sql("ALTER INSTANCE ROTATE INNODB MASTER KEY", this);
      break;
    case Option::ROTATE_REDO_LOG_KEY:
      execute_sql("SELECT rotate_system_key(\"percona_redo\")", this);
      break;
    case Option::ALTER_DATABASE_ENCRYPTION:
      alter_database_encryption(this);
      break;
    case Option::UNDO_SQL:
      create_alter_drop_undo(this);
      break;
    case Option::SPECIAL_SQL:
      special_sql(all_session_tables, this);
      break;

    default:
      throw std::runtime_error("invalid options");
    }

    options->at(option)->total_queries++;

    /* sql executed is at 0 index, and if successful at 1 */
    opt_feq[option][0]++;
    if (success) {
      options->at(option)->success_queries++;
      opt_feq[option][1]++;
      success = false;
    }

    if (connection_lost) {
      break;
    }
  } // while

  /* print options frequency in logs */
  for (int i = 0; i < Option::MAX; i++) {
    if (opt_feq[i][0] > 0)
      thread_log << options->at(i)->help << ", total=>" << opt_feq[i][0]
                 << ", success=> " << opt_feq[i][1] << std::endl;
  }

  /* cleanup session temporary tables tables */
  for (auto &table : *all_session_tables)
    if (table->type == Table::TEMPORARY)
      delete table;
  delete all_session_tables;
}
