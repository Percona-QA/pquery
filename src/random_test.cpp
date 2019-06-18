#include "random_test.hpp"
#include "node.hpp"

using namespace std;
using namespace rapidjson;
std::mt19937 rng;

/* PRIMARY has to be in last */
enum COLUMN_TYPES { INT, CHAR, VARCHAR, PRIMARY, MAX };
const std::string column_type[] = {"INT", "CHAR", "VARCHAR",
                                   "INT PRIMARY KEY AUTO_INCREMENT"};

std::vector<std::string> Thd1::encryption = {"Y", "N"};
std::vector<std::string> Thd1::row_format = {"COMPRESSED", "DYNAMIC"};
std::vector<int> Thd1::key_block_size = {0, 0, 1, 2, 4};
std::string Thd1::engine = "INNODB";
int Thd1::default_records_in_table = 1000;
int Thd1::no_of_tables = 3;
int Thd1::s_len = 32;
int Thd1::pkey_pb_per_table = 100;
int Thd1::no_of_random_load_per_thread = 10000;
int Thd1::ddl = true;

typedef std::vector<Thd1::opt *> thd_ops;

thd_ops *options_process() {
  thd_ops *v_ops = new thd_ops;
  v_ops->reserve(RANDOM_MAX);
  v_ops->push_back(new Thd1::opt(DROP_COLUMN, 2, true));
  v_ops->push_back(new Thd1::opt(ADD_COLUMN, 2, true));
  v_ops->push_back(new Thd1::opt(INSERT_RANDOM_ROW, 600, false));
  v_ops->push_back(new Thd1::opt(DROP_CREATE, 1, true));
  v_ops->push_back(new Thd1::opt(TRUNCATE, 2, true));
  v_ops->push_back(new Thd1::opt(OPTIMIZE, 15, false));
  v_ops->push_back(new Thd1::opt(ANALYZE, 24, false));
  v_ops->push_back(new Thd1::opt(DELETE_ALL_ROW, 0, false));
  v_ops->push_back(new Thd1::opt(ENCRYPTION, 40, true));
  v_ops->push_back(new Thd1::opt(DELETE_ROW_USING_PKEY, 0, false));
  v_ops->push_back(new Thd1::opt(UPDATE_ROW_USING_PKEY, 100, false));
  return v_ops;
};

thd_ops *Thd1::options = options_process();

std::vector<std::string> *random_strs_generator() {
  static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "0123456789";

  static const size_t N_STRS = 10000;

  std::random_device rd;
  std::default_random_engine rng(rd());
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

std::vector<std::string> *Thd1::random_strs = random_strs_generator();

int rand_int(int upper, int lower) {
  /*todo change the approach if it is too slow */
  std::uniform_int_distribution<std::mt19937::result_type> dist(
      lower, upper); // distribution in range [lower, upper]
  return dist(rng);
}

/* return random string in range of upper and lower */
string rand_string(int upper, int lower) {
  string rs = ""; /*random_string*/
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
vector<string> tablespace; // = {"abc"};
int version = 1;
string file_read_path = "data.dll";
string file_write_path = "new_data.dll";
int load_from_file = 0;
string partition_string = "_p";
int default_columns_in_table = 3;
int default_indexes_in_table = 5;
int default_columns_in_index = 2;
} // namespace ta


struct Column {
  Column(string n, Table *table) : name_(n), table_(table){};

  Column(string name, string type, bool n, int l, Table *table)
      : name_(name), null(n), length(l), table_(table) {
    for (int i = 0; i < COLUMN_TYPES::MAX; i++) {
      if (type.compare(column_type[i]) == 0) {
        type_ = i;
        break;
      }
    }
  };

  Column(string name, Table *table, int type)
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
      length = rand_int(200, 10);
      break;
    }
  };

  string rand_value();

  template <typename Writer> void Serialize(Writer &writer) const {
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

  ~Column(){};

  std::string name_;
  int type_;
  bool null = false;
  int length = 0;
  std::string default_value;
  bool primary_key = false;
  bool generated = false;
  Table *table_;
};

string Column::rand_value() {
  if (type_ == COLUMN_TYPES::INT)
    return to_string(rand_int(100, 4));
  else
    return "\'" + rand_string(length) + "\'";
}

struct Ind_col {
  Ind_col(Column *c) : column(c){};
  Ind_col(Column *c, bool d) : column(c), desc(d){};
  template <typename Writer> void Serialize(Writer &writer) const {
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

  Column *column;
  bool desc = false;
  int length = 0;
};

struct Index {

  Index(string n) : columns_(), name_(n) { columns_ = new vector<Ind_col *>; };
  vector<Ind_col *> *columns_;
  void AddInternalColumn(Ind_col *column) { columns_->push_back(column); }
  template <typename Writer> void Serialize(Writer &writer) const {
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

  std::string name_;
  ~Index() {
    for (auto id_col : *columns_) {
      delete id_col;
    }
    delete columns_;
  };
};

/* Partition table */
struct Partition_table : public Table {
public:
  Partition_table(string n, int max_pk) : Table(n, max_pk){};
  ~Partition_table() {}
  string partition_start;
  string partiton_type() { return partition_start; }
};

Table::Table(std::string n, int max_pk)
    : name_(n), indexes_(), max_pk_value_inserted(max_pk) {
  columns_ = new vector<Column *>;
  indexes_ = new vector<Index *>;
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

  for (int i = 0; i < rand_int(ta::default_columns_in_table, 1); i++) {
    string name;
    int type;
    /*  if we need to create primary column */
    if (i == 0 && rand_int(100) < Thd1::pkey_pb_per_table) {
      type = COLUMN_TYPES::PRIMARY;
      name = "pkey";
      has_pk = true;
    } else {
      name = "c" + to_string(i);
      type = rand_int(COLUMN_TYPES::MAX - 2); // DON"T PICK Serial
    }
    Column *col = new Column{name, this, type};
    AddInternalColumn(col);
  }
}

/* create default indexes */
void Table::CreateDefaultIndex() {

  int indexes = rand_int(ta::default_indexes_in_table, 1);
  for (int i = 0; i < indexes; i++) {

    Index *id = new Index(name_ + "i" + to_string(i));

    int max_columns = rand_int(ta::default_columns_in_index, 1);

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
  string name = "tt_" + to_string(id);
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

  if (table->key_block_size > 0 && rand_int(3) == 0)
    table->row_format = "COMPRESSED";

  if (table->key_block_size == 0)
    table->row_format = Thd1::row_format[rand_int(Thd1::row_format.size() - 1)];

  if (!Thd1::engine.empty())
    table->engine = Thd1::engine;

  if (ta::tablespace.size() > 0 && rand_int(10) > 5)
    table->tablespace = ta::tablespace[rand_int(ta::tablespace.size() - 1)];

  return table;
}

/* prepare table defination */
string Table::Table_defination() {
  std::string def = "CREATE TABLE  " + name_ + " (";

  // todo move to method
  for (auto col : *columns_) {
    def += " " + col->name_ + " " + column_type[col->type_];
    if (col->length > 0)
      def += "(" + to_string(col->length) + ")";
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
    def += " KEY_BLOCK_SIZE=" + to_string(key_block_size);

  if (row_format.size() > 0)
    def += " ROW_FORMAT=" + row_format;

  if (!engine.empty())
    def += " ENGINE=" + engine;

  def += ";";
  return def;
}
/* create default table include all tables now */
void create_default_tables(std::vector<Table *> *all_tables) {
  int no_of_tables = Thd1::no_of_tables;
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
  string sql = "ALTER TABLE " + name_ + " ENCRYPTION = '";
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
  string sql = "ALTER TABLE " + name_ + "  ADD COLUMN ";
  string name;
  name = "COL" + to_string(rand_int(300));

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
  string sql = "DELETE FROM " + name_ + ";";
  execute_sql(sql, thd);
}

void Table::DeleteRandomRow(Thd1 *thd) {
  if (has_pk) {
    auto pk = rand_int(max_pk_value_inserted);
    string sql = "DELETE FROM " + name_ + " WHERE pkey=" + to_string(pk) + ";";
    execute_sql(sql, thd);
  }
}

void Table::UpdateRandomROW(Thd1 *thd) {
  if (has_pk) {
    auto pk = rand_int(max_pk_value_inserted);
    string sql = "UPDATE " + name_ + " SET pkey =" + to_string(-pk);
    sql += " WHERE pkey= " + to_string(pk);
    execute_sql(sql, thd);
  }
}

void Table::InsertRandomRow(Thd1 *thd, bool is_lock) {
  if (is_lock)
    table_mutex.lock();
  string vals = "";
  string insert = "INSERT INTO " + name_ + "  ( ";
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
  if (name.compare("pkey") == 0) {
    table_mutex.unlock();
    return;
  }
  string sql = "ALTER TABLE " + name_ + " DROP COLUMN " + name + ";";
  table_mutex.unlock();

  if (execute_sql(sql, thd)) {
    table_mutex.lock();

    vector<int> indexes_to_drop;
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
    std::sort(indexes_to_drop.begin(), indexes_to_drop.end(), greater<int>());

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

/* save objects to a file */
void save_objects_to_file(vector<Table *> *all_tables) {
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
                             to_string(v) + " code::version is " +
                             to_string(version));

  for (auto &tab : d["tables"].GetArray()) {
    Table *table;

    string name = tab["name"].GetString();
    string table_type = name.substr(name.size() - 2, 2);

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
        string index_base_column = ind_col["name"].GetString();

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
    Thd1::no_of_tables = all_tables->size();
  }
  fclose(fp);
}

/* clean tables from memory */
void clean_up_at_end(std::vector<Table *> *all_tables) {
  for (auto &table : *all_tables)
    delete table;
}

int run_default_load(Thd1 *thd) {
  std::vector<Table *> *all_tables = thd->tables;
  auto &logs = thd->thread_log;
  std::random_device dev;
  std::mt19937 rng(dev());
  logs << " creating defaut tables " << endl;

  ta::load_from_file == 1 ? load_objects_from_file(all_tables)
                          : create_default_tables(all_tables);

  if (Thd1::no_of_tables <= 0)
    throw std::runtime_error("no table to work on \n");

  execute_sql("DROP DATABASE test;", thd);
  execute_sql("CREATE DATABASE test;", thd);
  execute_sql("USE test;", thd);
  for (auto &table : *all_tables) {
    execute_sql(table->Table_defination(), thd);
  }
  load_default_data(all_tables, thd);
  return 1;
}
int sum_of_all_options() {
  int total = 0;
  for (auto &opt : *Thd1::options) {
    if (!Thd1::ddl && opt->ddl)
      continue;
    total += opt->probability;
  }
  return total;
}

static int total_probablity = sum_of_all_options();

int pick_some_option() {
  int rd = rand_int(total_probablity);
  for (auto &opt : *Thd1::options) {
    if (!Thd1::ddl && opt->ddl)
      continue;
    if (rd <= opt->probability)
      return opt->type;
    else
      rd -= opt->probability;
  }
  return -1;
};

void run_some_query(Thd1 *thd) {
  auto &logs = thd->thread_log;
  auto all_tables = thd->tables;
  execute_sql("USE test;", thd);
  Node::parallel_thread_running++;
  for (int i = 0; i < Thd1::no_of_random_load_per_thread; i++) {
    auto size = all_tables->size();

    if (i % 20 == 0)
      logs << " executed " << i << " queries " << std::endl;

    auto table = all_tables->at(rand_int(size - 1));
    auto x = pick_some_option();
    switch (x) {
    case DROP_COLUMN:
      table->DropColumn(thd);
      break;
    case ADD_COLUMN:
      table->AddColumn(thd);
      break;
    case INSERT_RANDOM_ROW:
      table->InsertRandomRow(thd, true);
      break;
    case DROP_CREATE:
      table->DropCreate(thd);
      break;
    case TRUNCATE:
      table->Truncate(thd);
      break;
    case OPTIMIZE:
      table->Optimize(thd);
      break;
    case ANALYZE:
      table->Analyze(thd);
      break;
    case DELETE_ALL_ROW:
      table->DeleteAllRows(thd);
      break;
    case ENCRYPTION:
      table->SetEncryption(thd);
      break;
    case DELETE_ROW_USING_PKEY:
      table->DeleteRandomRow(thd);
      break;
    case UPDATE_ROW_USING_PKEY:
      table->UpdateRandomROW(thd);
      break;
    default:
      throw std::runtime_error("invalid options");
    }
  }
  Node::parallel_thread_running--;
}

