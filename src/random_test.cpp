#include "random_test.hpp"
#include "common.hpp"
#include "node.hpp"

using namespace rapidjson;
using namespace std;
std::mt19937 rng;

#define INNODB_16K_PAGE_SIZE 16
#define INNODB_8K_PAGE_SIZE 8
#define INNODB_32K_PAGE_SIZE 32
#define INNODB_64K_PAGE_SIZE 64
#define MIN_SEED_SIZE 10000
#define MAX_SEED_SIZE 100000

typedef std::vector<Thd1::opt *> thd_ops;

/* PRIMARY has to be in last */
enum COLUMN_TYPES { INT, CHAR, VARCHAR, PRIMARY, MAX };
const std::string column_type[] = {"INT", "CHAR", "VARCHAR",
                                   "INT PRIMARY KEY AUTO_INCREMENT"};

int sum_of_all_options() {
  int total = 0;

  /* if select is set as zero , disable all type of selects */
  if (options->at(Option::SELECT)->getBool() == false) {
    options->at(Option::SELECT_ALL_ROW)->setInt(0);
    options->at(Option::SELECT_ROW_USING_PKEY)->setInt(0);
  }

  bool ddl = options->at(Option::DDL)->getBool();
  for (auto &opt : *options) {
    if (opt == nullptr || !opt->sql || (!ddl && opt->ddl))
      continue;
    total += opt->getInt();
  }
  return total;
}

Option::Opt pick_some_option() {
  std::cout << "TRYING" << std::endl;
  static int total_probablity = sum_of_all_options();
  int rd = rand_int(total_probablity, 1);
  static bool ddl = options->at(Option::DDL)->getBool();
  for (auto &opt : *options) {
    if (opt == nullptr || !opt->sql || (!ddl && opt->ddl))
      continue;
    if (rd <= opt->getInt())
      return opt->getOption();
    else
      rd -= opt->getInt();
  }
  return Option::MAX;
};

/* set seed of current thread */
int set_seed(Thd1 *thd) {
  auto initial_seed = Thd1::initial_seed;
  std::default_random_engine rng(initial_seed);
  std::uniform_int_distribution<std::mt19937::result_type> dis(MIN_SEED_SIZE,
                                                               MAX_SEED_SIZE);
  for (int i = 0; i < thd->thread_id; i++)
    dis(rng);
  thd->seed = dis(rng);
  thd->thread_log << "CURRENT SEED IS " << thd->seed << std::endl;
  return thd->seed;
}

std::vector<std::string> Thd1::encryption = {"Y", "N"};
std::vector<std::string> Thd1::row_format = {"COMPRESSED", "DYNAMIC",
                                             "REDUNDANT"};
std::vector<std::string> Thd1::tablespace = {"innodb_system", "tab02k",
                                             "tab04k", "tab01k"};
std::vector<int> Thd1::key_block_size = {0, 0, 1, 2, 4};
std::string Thd1::engine = "INNODB";
int Thd1::default_records_in_table = 10;
int Thd1::s_len = 32;
int Thd1::no_of_random_load_per_thread = 1000;
int Thd1::pkey_pb_per_table = 100;
int Thd1::ddl = true;
bool Thd1::is_innodb_system_encrypted = false;
int Thd1::max_columns_length = 100;
int Thd1::max_columns_in_table = 3;
int Thd1::max_indexes_in_table = 2;
int Thd1::max_columns_in_index = 2;
int Thd1::innodb_page_size = 16;
bool Thd1::just_load_ddl = false;
unsigned long int Thd1::initial_seed = 5;

thd_ops *options_process() {
  thd_ops *v_ops = new thd_ops;
  /*
  v_ops->reserve(RANDOM_MAX);
  v_ops->push_back(new Thd1::opt(DROP_CREATE, 1, true));
  v_ops->push_back(new Thd1::opt(OPTIMIZE, 15, false));
  v_ops->push_back(new Thd1::opt(ANALYZE, 24, false));
  v_ops->push_back(new Thd1::opt(DELETE_ALL_ROW, 1, false));
  v_ops->push_back(new Thd1::opt(SELECT_ALL_ROW, 1, false));
  v_ops->push_back(new Thd1::opt(ENCRYPTION, 40, true));
  v_ops->push_back(new Thd1::opt(TABLESPACE_ENCRYPTION, 40, true));
  v_ops->push_back(new Thd1::opt(TABLESPACE_RENAME, 40, true));
  v_ops->push_back(new Thd1::opt(COLUMN_RENAME, 40, true));
  v_ops->push_back(new Thd1::opt(DELETE_ROW_USING_PKEY, 0, false));
  v_ops->push_back(new Thd1::opt(UPDATE_ROW_USING_PKEY, 100, false));
  v_ops->push_back(new Thd1::opt(SELECT_ROW_USING_PKEY, 100, false));
  */
  return v_ops;
};

thd_ops *Thd1::options = options_process();

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
    str.reserve(Thd1::s_len);
    std::generate_n(std::back_inserter(str), Thd1::s_len,
                    [&]() { return alphabet[dist(rng)]; });

    return str;
  });
  return strs;
}

std::vector<std::string> *Thd1::random_strs =
    random_strs_generator(Thd1::initial_seed);

int rand_int(int upper, int lower) {
  /*todo change the approach if it is too slow */
  std::uniform_int_distribution<std::mt19937::result_type> dist(
      lower, upper); // distribution in range [lower, upper]
  return dist(rng);
}

/* return random string in range of upper and lower */
std::string rand_string(int upper, int lower) {
  std::string rs = ""; /*random_string*/
  auto size = rand_int(upper, lower);

  /* let the query fail with string size greater */
  if (rand_int(100) < 3)
    size *= 1;

  while (size > 0) {
    auto str = Thd1::random_strs->at(rand_int(Thd1::random_strs->size() - 1));
    if (size > Thd1::s_len)
      rs += str;
    else
      rs += str.substr(0, size);
    size -= Thd1::s_len;
  }
  return rs;
}

/* table attirbutes */
namespace ta {
int version = 1;
std::string file_read_path = "data.dll";
std::string file_write_path = "new_data.dll";
int load_from_file = 0;
std::string partition_string = "_p";
} // namespace ta

std::string Column::rand_value() {
  if (type_ == COLUMN_TYPES::INT)
    return std::to_string(rand_int(100, 4));
  else
    return "\'" + rand_string(length) + "\'";
}

Column::Column(std::string name, Table *table) : name_(name), table_(table){};

Column::Column(std::string name, std::string type, bool is_null, int len,
               Table *table)
    : name_(name), null(is_null), length(len), table_(table) {
  for (int i = 0; i < COLUMN_TYPES::MAX; i++) {
    if (type.compare(column_type[i]) == 0) {
      type_ = i;
      break;
    }
  }
};

Column::Column(std::string name, Table *table, int type)
    : name_(name), type_(type), table_(table) {
  switch (type) {
  case (COLUMN_TYPES::INT):
    if (rand_int(10) == 1)
      length = rand_int(100, 20);
    else
      length = 0;
    break;
  case (COLUMN_TYPES::VARCHAR):
  case (COLUMN_TYPES::CHAR):
    length = rand_int(Thd1::max_columns_length, 10);
    break;
  }
};

template <typename Writer> void Column::Serialize(Writer &writer) const {
  writer.StartObject();
  writer.String("name");
  writer.String(name_.c_str(), static_cast<SizeType>(name_.length()));
  writer.String("type");
  writer.String(column_type[type_].c_str(),
                static_cast<SizeType>(column_type[type_].length()));
  writer.String("null");
  writer.Bool(null);
  writer.String("primary_key");
  writer.Bool(primary_key);
  writer.String("generated");
  writer.Bool(generated);
  writer.String("lenght");
  writer.Int(length);
  writer.EndObject();
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
};

template <typename Writer> void Table::Serialize(Writer &writer) const {
  writer.StartObject();
  writer.String("name");
#if RAPIDJSON_HAS_STDSTRING
  writer.String(name_);
#else
  writer.String(name_.c_str(), static_cast<SizeType>(name_.length()));
#endif
  if (has_pk) {
    writer.String("max_pk_value_inserted");
    writer.Int(max_pk_value_inserted);
  }

  if (!tablespace.empty()) {
    writer.String("tablespace");
    writer.String(tablespace.c_str(),
                  static_cast<SizeType>(tablespace.length()));
  }

  writer.String(("columns"));
  writer.StartArray();
  for (auto &col : *columns_)
    col->Serialize(writer);
  writer.EndArray();
  writer.String(("indexes"));
  writer.StartArray();
  for (auto *ind : *indexes_)
    ind->Serialize(writer);
  writer.EndArray();
  writer.EndObject();
}

Ind_col::Ind_col(Column *c) : column(c){};

Ind_col::Ind_col(Column *c, bool d) : column(c), desc(d){};

Index::Index(std::string n) : name_(n), columns_() {
  columns_ = new std::vector<Ind_col *>;
};

void Index::AddInternalColumn(Ind_col *column) { columns_->push_back(column); }

Table::Table(std::string n, int max_pk)
    : name_(n), indexes_(), max_pk_value_inserted(max_pk) {
  columns_ = new std::vector<Column *>;
  indexes_ = new std::vector<Index *>;
};

void Table::DropCreate(Thd1 *thd) {
  if (execute_sql("drop table " + name_ + ";", thd))
    max_pk_value_inserted = 0;
  if (execute_sql(Table_defination(), thd))
    max_pk_value_inserted = 0;
}

void Table::Optimize(Thd1 *thd) {
  execute_sql("OPTIMIZE TABLE " + name_ + ";", thd);
}

void Table::Analyze(Thd1 *thd) {
  execute_sql("ANALYZE TABLE " + name_ + ";", thd);
}

void Table::Truncate(Thd1 *thd) {
  if (execute_sql("TRUNCATE TABLE " + name_ + ";", thd))
    max_pk_value_inserted = 0;
}

Table::~Table() {
  for (auto ind : *indexes_)
    delete ind;
  for (auto col : *columns_) {
    delete col;
  }
  delete columns_;
  delete indexes_;
};

/* create default column */
void Table::CreateDefaultColumn() {

  for (int i = 0; i < rand_int(Thd1::max_columns_in_table, 1); i++) {
    std::string name;
    int type;
    /*  if we need to create primary column */
    if (i == 0 && rand_int(100) < Thd1::pkey_pb_per_table) {
      type = COLUMN_TYPES::PRIMARY;
      name = "pkey";
      has_pk = true;
    } else {
      name = "c" + std::to_string(i);
      type = rand_int(COLUMN_TYPES::MAX - 2); // DON"T PICK Serial
    }
    Column *col = new Column{name, this, type};
    AddInternalColumn(col);
  }
}

/* create default indexes */
void Table::CreateDefaultIndex() {

  int indexes = rand_int(Thd1::max_indexes_in_table, 1);
  for (int i = 0; i < indexes; i++) {

    Index *id = new Index(name_ + "i" + std::to_string(i));

    int max_columns = rand_int(Thd1::max_columns_in_index, 1);

    int columns = max_columns;

    for (auto *col : *columns_) {
      if (rand_int(3) > 1)
        continue;
      else {
        if (rand_int(2) == 0)
          id->AddInternalColumn(new Ind_col(col)); // desc is set false
        else
          id->AddInternalColumn(new Ind_col(col, true)); // desc is set as true
        if (--columns == 0)
          break;
      }
    }

    /* make sure it has  atleast one column */
    if (columns == max_columns)
      id->AddInternalColumn(new Ind_col(columns_->at(0)));
    AddInternalIndex(id);
  }
}

/* Create new table and pick some attributes */
Table *Table::table_id(int choice, int id) {
  Table *table;
  std::string name = "tt_" + std::to_string(id);
  if (choice == PARTITION) {
    table = new Partition_table(name + ta::partition_string, 0);
  } else {
    table = new Table(name, 0);
  }

  table->CreateDefaultColumn();
  table->CreateDefaultIndex();
  if (Thd1::encryption.size() > 0 &&
      Thd1::encryption[rand_int(Thd1::encryption.size() - 1)].compare("Y") == 0)
    table->encryption = true;

  if (Thd1::key_block_size.size() > 0)
    table->key_block_size =
        Thd1::key_block_size[rand_int(Thd1::key_block_size.size() - 1)];

  if (table->key_block_size > 0 && rand_int(2) == 0) {
    table->row_format = "COMPRESSED";
  }

  if (table->key_block_size == 0 && Thd1::row_format.size() > 0)
    table->row_format = Thd1::row_format[rand_int(Thd1::row_format.size() - 1)];

  if (!Thd1::engine.empty())
    table->engine = Thd1::engine;

  if (Thd1::tablespace.size() > 0 && rand_int(2) == 0) {
    table->tablespace = Thd1::tablespace[rand_int(Thd1::tablespace.size() - 1)];
    table->encryption = false;
    table->row_format.clear();
    if (Thd1::innodb_page_size > INNODB_16K_PAGE_SIZE ||
        table->tablespace.compare("innodb_system") == 0 ||
        stoi(table->tablespace.substr(3, 2)) == Thd1::innodb_page_size)
      table->key_block_size = 0;
    else
      table->key_block_size = std::stoi(table->tablespace.substr(3, 2));
  }

  return table;
}

/* prepare table defination */
std::string Table::Table_defination() {
  std::string def = "CREATE TABLE  " + name_ + " (";

  // todo move to method
  for (auto col : *columns_) {
    def += " " + col->name_ + " " + column_type[col->type_];
    if (col->length > 0)
      def += "(" + std::to_string(col->length) + ")";
    if (col->null)
      def += " NOT NULL";
    def += ", ";
  }

  for (auto id : *indexes_) {
    def += "INDEX " + id->name_ + "(";
    for (auto idc : *id->columns_) {
      def += idc->column->name_;
      def += (idc->desc ? " DESC" : (rand_int(3) ? "" : " ASC"));
      def += ", ";
    }
    def.erase(def.length() - 2);
    def += "), ";
  }
  def.erase(def.length() - 2);
  def += " )";

  if (encryption)
    def += " ENCRYPTION='Y'";
  else if (rand_int(3) == 1)
    def += " ENCRYPTION='N'";

  if (!tablespace.empty())
    def += " TABLESPACE=" + tablespace;

  if (key_block_size > 0)
    def += " KEY_BLOCK_SIZE=" + std::to_string(key_block_size);

  if (row_format.size() > 0)
    def += " ROW_FORMAT=" + row_format;

  if (!engine.empty())
    def += " ENGINE=" + engine;

  def += ";";
  return def;
}
/* create default table include all tables now */
void create_default_tables(std::vector<Table *> *all_tables) {
  int no_of_tables = options->at(Option::TABLE)->getInt();
  for (int i = 0; i < no_of_tables; i++) {
    Table *t = Table::table_id(rand_int(TABLE_MAX), i);
    // auto *n = static_cast<Partition_table *>(t);
    all_tables->push_back(t);
  }
}

int execute_sql(std::string sql, Thd1 *thd) {
  auto query = sql.c_str();
  auto res = mysql_real_query(thd->conn, query, strlen(query));
  if (res == 1) {
    std::cout << "Query => " << sql << std::endl;
    std::cout << "Error " << mysql_error(thd->conn) << std::endl;
  } else {
    MYSQL_RES *result;
    result = mysql_store_result(thd->conn);
    mysql_free_result(result);
  }
  return (res == 0 ? 1 : 0);
}

void load_default_data(std::vector<Table *> *all_tables, Thd1 *thd) {
  for (auto i = all_tables->begin(); i != all_tables->end(); i++) {
    auto table = *i;
    int rec = rand_int(Thd1::default_records_in_table);
    for (int i = 0; i < rec; i++) {
      table->InsertRandomRow(thd, false);
    }
  }
}

void Table::SetEncryption(Thd1 *thd) {
  std::string sql = "ALTER TABLE " + name_ + " ENCRYPTION = '";
  std::string enc = thd->encryption[rand_int(thd->encryption.size() - 1)];
  sql += enc + "';";
  if (execute_sql(sql, thd)) {
    table_mutex.lock();
    if (enc.compare("Y"))
      encryption = true;
    else
      encryption = false;
    table_mutex.unlock();
  }
};

/* alter table add column */
void Table::AddColumn(Thd1 *thd) {
  std::string sql = "ALTER TABLE " + name_ + "  ADD COLUMN ";
  std::string name;
  name = "COL" + std::to_string(rand_int(300));

  auto col = rand_int(COLUMN_TYPES::MAX - 2);
  Column *tc = new Column(name, this, col);

  sql += name + " " + column_type[tc->type_];
  if (tc->length > 0)
    sql += "(" + std::to_string(tc->length) + ")";
  sql += ";";

  if (execute_sql(sql, thd)) {
    table_mutex.lock();
    AddInternalColumn(tc);
    table_mutex.unlock();
  } else
    delete tc;
}

void Table::DeleteAllRows(Thd1 *thd) {
  std::string sql = "DELETE FROM " + name_ + ";";
  execute_sql(sql, thd);
}

void Table::SelectAllRow(Thd1 *thd) {
  std::string sql = "SELECT * FROM " + name_ + ";";
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
  };
}

void Table::DeleteRandomRow(Thd1 *thd) {
  if (has_pk) {
    auto pk = rand_int(max_pk_value_inserted);
    std::string sql =
        "DELETE FROM " + name_ + " WHERE pkey=" + std::to_string(pk) + ";";
    execute_sql(sql, thd);
  }
}

void Table::SelectRandomRow(Thd1 *thd) {
  if (has_pk) {
    auto pk = rand_int(max_pk_value_inserted);
    table_mutex.lock();
    std::string sql = "SELECT * FROM " + name_ + " WHERE " +
                      columns_->at(0)->name_ + "=" + std::to_string(pk) + ";";
    table_mutex.unlock();
    execute_sql(sql, thd);
  }
}
void Table::UpdateRandomROW(Thd1 *thd) {
  if (has_pk) {
    if (max_pk_value_inserted == 0)
      return;
    auto pk = rand_int(max_pk_value_inserted);
    table_mutex.lock();
    std::string sql = "UPDATE " + name_ + " SET " + columns_->at(0)->name_ +
                      "=" + std::to_string(-pk) + " WHERE " +
                      columns_->at(0)->name_ + "=" + std::to_string(pk);
    table_mutex.unlock();
    execute_sql(sql, thd);
  }
}

void Table::InsertRandomRow(Thd1 *thd, bool is_lock) {
  if (is_lock)
    table_mutex.lock();
  std::string vals = "";
  std::string insert = "INSERT INTO " + name_ + "  ( ";
  for (auto &column : *columns_) {
    if (column->type_ == COLUMN_TYPES::PRIMARY)
      continue;
    insert += column->name_ + " ,";
    vals += " " + column->rand_value() + ",";
  }

  if (vals.size() > 0) {
    vals.pop_back();
    insert.pop_back();
  }
  insert += ") VALUES(" + vals;
  insert += " );";
  if (is_lock)
    table_mutex.unlock();
  if (execute_sql(insert, thd))
    max_pk_value_inserted++;
}

/* alter table drop column */
void Table::DropColumn(Thd1 *thd) {
  table_mutex.lock();
  if (columns_->size() == 1) {
    table_mutex.unlock();
    return;
  }
  auto ps = rand_int(columns_->size() - 1);
  auto name = columns_->at(ps)->name_;
  if (name.compare("pkey") == 0 || name.compare("pkey_rename") == 0) {
    table_mutex.unlock();
    return;
  }
  std::string sql = "ALTER TABLE " + name_ + " DROP COLUMN " + name + ";";
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
      if ((*pos)->name_.compare(name) == 0) {
        delete *pos;
        columns_->erase(pos);
        break;
      }
    }
    table_mutex.unlock();
  }
}

void alter_tablespace_encryption(Thd1 *thd) {
  if (Thd1::tablespace.size() > 0) {
    std::string sql = "ALTER tablespace " +
                      Thd1::tablespace[rand_int(Thd1::tablespace.size() - 1)] +
                      " ENCRYPTION ";
    sql += (rand_int(1) == 0 ? "'y'" : "'n'");
    sql += ";";
    execute_sql(sql, thd);
  }
};

void alter_tablespace_rename(Thd1 *thd) {
  if (Thd1::tablespace.size() > 0) {
    auto tablespace = Thd1::tablespace[rand_int(Thd1::tablespace.size() - 1),
                                       1]; // don't pick innodb_system;
    std::string sql = "ALTER tablespace " + tablespace;
    if (rand_int(1) == 0)
      sql += "_rename  rename to " + tablespace + ";";
    else
      sql += " rename to " + tablespace + "_rename;";
    execute_sql(sql, thd);
  }
};

/* save objects to a file */
void save_objects_to_file(std::vector<Table *> *all_tables) {
  //  rapidjson::Writer<Stream> writer(stream);

  StringBuffer sb;
  PrettyWriter<StringBuffer> writer(sb);
  writer.StartObject();
  writer.String("version");
  writer.Uint(ta::version);

  writer.String(("tables"));
  writer.StartArray();
  for (auto j = all_tables->begin(); j != all_tables->end(); j++) {
    auto table = *j;
    table->Serialize(writer);
  }
  writer.EndArray();
  writer.EndObject();
  std::ofstream of(ta::file_write_path);
  of << sb.GetString();
  if (!of.good())
    throw std::runtime_error("Can't write the JSON string to the file!");
}

/*load objects from a file */
void load_objects_from_file(std::vector<Table *> *all_tables) {
  int version = ta::version;
  FILE *fp = fopen(ta::file_read_path.c_str(), "r");
  char readBuffer[65536];
  FileReadStream is(fp, readBuffer, sizeof(readBuffer));
  Document d;
  d.ParseStream(is);
  auto v = d["version"].GetInt();

  if (d["version"].GetInt() != version)
    throw std::runtime_error("version mismatch between " + ta::file_read_path +
                             " and codebase " + " file::version is " +
                             std::to_string(v) + " code::version is " +
                             std::to_string(version));

  for (auto &tab : d["tables"].GetArray()) {
    Table *table;

    std::string name = tab["name"].GetString();
    std::string table_type = name.substr(name.size() - 2, 2);

    int max_pk = tab["max_pk_value_inserted"].GetInt();

    if (table_type.compare(ta::partition_string) == 0)
      table = new Partition_table(name, max_pk);
    else
      table = new Table(name, max_pk);

    for (auto &col : tab["columns"].GetArray()) {
      Column *a =
          new Column(col["name"].GetString(), col["type"].GetString(),
                     col["null"].GetBool(), col["lenght"].GetInt(), table);
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
    options->at(Option::DDL)->setInt(all_tables->size());
  }
  fclose(fp);
}

/* clean tables from memory */
void clean_up_at_end(std::vector<Table *> *all_tables) {
  for (auto &table : *all_tables)
    delete table;

  delete Thd1::random_strs;

  for (auto opt : *Thd1::options)
    delete opt;
  delete Thd1::options;
}

void create_database_tablespace(Thd1 *thd) {

  if (Thd1::innodb_page_size > INNODB_16K_PAGE_SIZE) {
    Thd1::row_format.clear();
    Thd1::key_block_size.clear();
  }

  if (Thd1::innodb_page_size >= INNODB_8K_PAGE_SIZE) {
    Thd1::tablespace.push_back("tab08k");
  }

  if (Thd1::innodb_page_size >= INNODB_16K_PAGE_SIZE) {
    Thd1::tablespace.push_back("tab16k");
  }

  if (Thd1::innodb_page_size >= INNODB_32K_PAGE_SIZE) {
    Thd1::tablespace.push_back("tab32k");
  }

  if (Thd1::innodb_page_size >= INNODB_64K_PAGE_SIZE) {
    Thd1::tablespace.push_back("tab64k");
  }
  execute_sql("DROP DATABASE test;", thd);
  execute_sql("CREATE DATABASE test;", thd);
  execute_sql("USE test;", thd);

  for (auto &tab : Thd1::tablespace) {
    if (tab.compare("innodb_system") == 0)
      continue;

    std::string sql =
        "CREATE TABLESPACE " + tab + " ADD DATAFILE '" + tab + ".ibd'";

    if (Thd1::innodb_page_size <= INNODB_16K_PAGE_SIZE) {
      sql += "FILE_BLOCK_SIZE " + tab.substr(3, 3);
    }

    sql += ";";
    execute_sql("DROP TABLESPACE " + tab + ";", thd);
    execute_sql(sql, thd);
  }
}

int run_default_load(Thd1 *thd) {
  std::vector<Table *> *all_tables = thd->tables;
  auto &logs = thd->thread_log;
  std::mt19937 rng(thd->initial_seed);

  logs << " creating defaut tables " << std::endl;

  ta::load_from_file == 1 ? load_objects_from_file(all_tables)
                          : create_default_tables(all_tables);

  create_database_tablespace(thd);

  if (options->at(Option::TABLE)->getInt() <= 0)
    throw std::runtime_error("no table to work on \n");

  for (auto &table : *all_tables) {
    execute_sql(table->Table_defination(), thd);
  }
  load_default_data(all_tables, thd);
  return 1;
}

void run_some_query(Thd1 *thd) {

  std::mt19937 rng(set_seed(thd));

  auto all_tables = thd->tables;

  execute_sql("USE test;", thd);

  Node::parallel_thread_running++;

  for (int i = 0; i < Thd1::no_of_random_load_per_thread; i++) {
    auto size = all_tables->size();

    auto table = all_tables->at(rand_int(size - 1));

    auto option = pick_some_option();
    thd->thread_log << "option picked is " << options->at(option)->getName()
                    << std::endl;
    switch (option) {
    case Option::DROP_COLUMN:
      table->DropColumn(thd);
      break;
    case Option::TRUNCATE:
      table->Truncate(thd);
      break;
    case Option::ADD_COLUMN:
      table->AddColumn(thd);
      break;
    case Option::DROP_CREATE:
      table->DropCreate(thd);
      break;
    case Option::ENCRYPTION:
      table->SetEncryption(thd);
      break;
    case Option::TABLESPACE_ENCRYPTION:
      alter_tablespace_encryption(thd);
      break;
    case Option::TABLESPACE_RENAME:
      alter_tablespace_rename(thd);
      break;
    case Option::SELECT_ALL_ROW:
      table->SelectAllRow(thd);
      break;
    case Option::SELECT_ROW_USING_PKEY:
      table->SelectRandomRow(thd);
      break;
    case Option::INSERT_RANDOM_ROW:
      table->InsertRandomRow(thd, true);
      break;
    case Option::DELETE_ALL_ROW:
      table->DeleteAllRows(thd);
      break;
    case Option::DELETE_ROW_USING_PKEY:
      table->DeleteRandomRow(thd);
      break;
    case Option::UPDATE_ROW_USING_PKEY:
      table->UpdateRandomROW(thd);
      break;
    case Option::OPTIMIZE:
      table->Optimize(thd);
      break;
    case Option::ANALYZE:
      table->Analyze(thd);
    case Option::RENAME_COLUMN:
      table->ColumnRename(thd);
      break;
    default:
      throw std::runtime_error("invalid options");
    }
  }
  Node::parallel_thread_running--;
}

