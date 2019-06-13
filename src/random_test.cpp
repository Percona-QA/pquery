#include "random_test.hpp"
#include "node.hpp"

using namespace std;
using namespace rapidjson;
std::mt19937 rng;

int rand_int(int upper, int lower) {
  /*todo change the approach if it is too slow */
  std::uniform_int_distribution<std::mt19937::result_type> dist(
      lower, upper); // distribution in range [lower, upper]
  return dist(rng);
}

string rand_string(int upper, int lower) {
  if (upper != lower)
    return "test";
  else
    return "FAIL";
}

/* table attirbutes */
namespace ta {
string encryption = "none";
vector<string> key_block_size;
string engine = "InnoDB";
vector<string> tablespace; // = {"abc"};
int no_of_tables = 10;
int version = 1;
string file_read_path = "data.dll";
string file_write_path = "new_data.dll";
int load_from_file = 0;
string partition_string = "_p";
int default_columns_in_table = 3;
int default_indexes_in_table = 5;
int default_columns_in_index = 2;
int default_number_of_records_in_table = 1;
vector<Table *> *all_tables;
} // namespace ta

/* Different table type supported by tool */
enum TABLE_TYPES { PARTITION, NORMAL };
/* Column Basic Properties */

enum COLUMN_TYPES { INT, CHAR, VARCHAR, MAX };

struct Column {
  Column(string n, Table *table) : name_(n), table_(table){};

  Column(string name, string type, bool n, int l, Table *table)
      : name_(name), type_(type), null(n), length(l), table_(table){};

  Column(string name, Table *table, int type) : name_(name), table_(table) {
    switch (type) {
    case (COLUMN_TYPES::INT):
      type_ = "INT";
      if (rand_int(10) == 1)
        length = rand_int(100, 20);
      else
        length = 0;
      break;
    case (COLUMN_TYPES::VARCHAR):
      type_ = "VARCHAR";
      length = rand_int(200, 10);
      break;
    case (COLUMN_TYPES::CHAR):
      type_ = "CHAR";
      length = rand_int(200, 10);
    }
  };

  string rand_value();

  template <typename Writer> void Serialize(Writer &writer) const {
    writer.StartObject();
    writer.String("name");
    writer.String(name_.c_str(), static_cast<SizeType>(name_.length()));
    writer.String("type");
    writer.String(type_.c_str(), static_cast<SizeType>(type_.length()));
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
  std::string type_;
  bool null = false;
  int length = 0;
  std::string default_value;
  bool primary_key = false;
  bool generated = false;
  Table *table_;
};

string Column::rand_value() {
  if (type_.compare("INT") == 0)
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

class Index {

public:
  Index(string n) : columns_(), name_(n) { columns_ = new vector<Ind_col *>; };
  vector<Ind_col *> *columns_;
  void AddColumn(Ind_col *column) { columns_->push_back(column); }
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

/* Table basic properties */
struct Table {
public:
  string engine;
  TABLE_TYPES table_type;
  Table(std::string n) : name_(n), indexes_() {
    columns_ = new vector<Column *>;
    indexes_ = new vector<Index *>;
  };
  std::string table_name() { return name_; };
  /* method to create table of choice */
  static Table *table_id(int choice, int id);
  string Table_defination();
  void AddColumn(Column *column) { columns_->push_back(column); }
  void AddIndex(Index *index) { indexes_->push_back(index); }
  void CreateDefaultColumn();
  void CreateDefaultIndex();
  void DropCreate(Thd1 *thd) {
    execute_sql("drop table " + name_ + ";", thd->conn);
    execute_sql(Table_defination(), thd->conn);
  }
  void Optimize(Thd1 *thd) {
    execute_sql("OPTIMIZE TABLE " + name_ + ";", thd->conn);
  }
  void Analyze(Thd1 *thd) {
    execute_sql("ANALYZE TABLE " + name_ + ";", thd->conn);
  }
  void Truncate(Thd1 *thd) {
    execute_sql("TRUNCATE TABLE " + name_ + ";", thd->conn);
  }
  template <typename Writer> void Serialize(Writer &writer) const {
    writer.StartObject();
    writer.String("name");
#if RAPIDJSON_HAS_STDSTRING
    writer.String(name_);
#else
    writer.String(name_.c_str(), static_cast<SizeType>(name_.length()));
#endif
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

  virtual ~Table() {
    for (auto ind : *indexes_)
      delete ind;
    for (auto col : *columns_) {
      delete col;
    }

    delete columns_;
    delete indexes_;
  };

  std::string name_;
  bool encryption = false;
  std::string key_block_size;
  std::string data_directory;
  std::string compression;
  std::string tablespace;
  std::vector<Column *> *columns_;
  std::vector<Index *> *indexes_;
  std::mutex table_mutex;
};

/* Partition table */
class Partition_table : public Table {
public:
  Partition_table(string n) : Table(n){};
  ~Partition_table() {}
  string partition_start;
  string partiton_type() { return partition_start; }
};

/* create default column */
void Table::CreateDefaultColumn() {
  int j = rand_int(ta::default_columns_in_table, 1);
  for (int i = 0; i < j; i++) {
    string name = "c" + to_string(i);
    auto val = rand_int(COLUMN_TYPES::MAX - 1);
    Column *a = new Column{name, this, val};
    AddColumn(a);
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
          id->AddColumn(new Ind_col(col)); // desc is set false
        else
          id->AddColumn(new Ind_col(col, true)); // desc is set as true
        if (--columns == 0)
          break;
      }
    }

    /* make sure it has  atleast one column */
    if (columns == max_columns)
      id->AddColumn(new Ind_col(columns_->at(0)));
    AddIndex(id);
  }
}

/* Create new table and add attributes */
Table *Table::table_id(int choice, int id) {
  Table *table;
  string name = "tt_" + to_string(id);
  if (choice == PARTITION) {
    table = new Partition_table(name + ta::partition_string);
  } else {
    table = new Table(name);
  }

  table->CreateDefaultColumn();
  table->CreateDefaultIndex();

  if (ta::encryption.compare("all") == 0 && rand_int(2) == 0)
    table->encryption = true;

  if (ta::key_block_size.size() > 0)
    table->key_block_size =
        ta::key_block_size[rand_int(ta::key_block_size.size() - 1)];

  if (!ta::engine.empty())
    table->engine = ta::engine;

  if (ta::tablespace.size() > 0 && rand_int(10) > 5)
    table->tablespace = ta::tablespace[rand_int(ta::tablespace.size() - 1)];

  return table;
}

/* prepare table defination */
string Table::Table_defination() {
  std::string def = "CREATE TABLE  " + name_ + " (";

  // todo move to method
  for (auto col : *columns_) {
    def += " " + col->name_ + " " + col->type_;
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
    def += " ENCRYPTION=Y";
  else if (rand_int(5) == 14)
    def += " ENCRYPTION=N";

  if (!tablespace.empty())
    def += ", tablespace=" + tablespace;

  if (!key_block_size.empty())
    def += ", KEY_BLOCK_SIZE=" + key_block_size;

  if (!engine.empty())
    def += " ENGINE " + engine;

  def += ";";
  return def;
}
/* create default table include all tables now */
void create_default_tables(std::vector<Table *> *all_tables) {
  int no_of_tables = ta::no_of_tables;
  for (int i = 0; i < no_of_tables; i++) {
    Table *t = Table::table_id(rand_int(2), i);
    // auto *n = static_cast<Partition_table *>(t);
    all_tables->push_back(t);
  }
}

int execute_sql(std::string sql, MYSQL *conn) {
  auto query = sql.c_str();
  auto res = mysql_real_query(conn, query, strlen(query));
  if (res == 1) {
    cout << " QUery => " << sql << endl;
    cout << "response " << res << endl;
  }

  return (res == 0 ? 1 : 0);
}

void load_default_data(std::vector<Table *> *all_tables, MYSQL *conn) {
  for (auto i = all_tables->begin(); i != all_tables->end(); i++) {
    auto table = *i;
    int rec = rand_int(ta::default_number_of_records_in_table);
    for (int i = 0; i <= rec; i++) {
      string vals = "";
      string insert = "INSERT INTO " + table->name_ + "  ( ";
      for (auto &column : *table->columns_) {
        insert += column->name_ + " ,";
        vals += " " + column->rand_value() + ",";
      }
      vals.pop_back();
      insert.pop_back();
      insert += ") VALUES(" + vals;
      insert += " );";
      execute_sql(insert, conn);
    }
  }
}

/* alter table add column */
void at_add_column(Table *table, MYSQL *conn) {
  string sql = "ALTER TABLE " + table->name_ + "  ADD COLUMN ";
  string name;
  name = "COL" + to_string(rand_int(300));

  auto col = rand_int(COLUMN_TYPES::MAX - 1);
  Column *tc = new Column(name, table, col);

  sql += name + " " + tc->type_;
  if (tc->length > 0)
    sql += "(" + std::to_string(tc->length) + ")";
  sql += ";";

  if (execute_sql(sql, conn)) {
    table->table_mutex.lock();
    tc->type_ = "INT";
    table->AddColumn(tc);
    table->table_mutex.unlock();
  } else
    delete tc;
}

void insert_data(Table *table, MYSQL *conn) {
  table->table_mutex.lock();
  string vals = "";
  string insert = "INSERT INTO " + table->name_ + "  ( ";
  for (auto &column : *table->columns_) {
    insert += column->name_ + " ,";
    vals += " " + column->rand_value() + ",";
  }

  vals.pop_back();
  insert.pop_back();
  insert += ") VALUES(" + vals;
  insert += " );";
  table->table_mutex.unlock();
  execute_sql(insert, conn);
}

/* alter table drop column */
void at_drop_column(Table *table, MYSQL *conn) {
  table->table_mutex.lock();
  if (table->columns_->size() == 1) {
    table->table_mutex.unlock();
    return;
  }
  auto ps = rand_int(table->columns_->size() - 1);
  auto name = table->columns_->at(ps)->name_;
  string sql = "ALTER TABLE " + table->name_ + " DROP COLUMN " + name + ";";
  table->table_mutex.unlock();

  if (execute_sql(sql, conn)) {
    table->table_mutex.lock();

    vector<int> indexes_to_drop;
    for (auto id = table->indexes_->begin(); id != table->indexes_->end();
         id++) {
      auto index = *id;

      for (auto id_col = index->columns_->begin();
           id_col != index->columns_->end(); id_col++) {
        auto ic = *id_col;
        if (ic->column->name_.compare(name) == 0) {
          if (index->columns_->size() == 1) {
            delete index;
            indexes_to_drop.push_back(id - table->indexes_->begin());
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
      table->indexes_->at(i) = table->indexes_->back();
      table->indexes_->pop_back();
    }
    // table->indexes_->erase(id);

    for (auto pos = table->columns_->begin(); pos != table->columns_->end();
         pos++) {
      if ((*pos)->name_.compare(name) == 0) {
        delete *pos;
        table->columns_->erase(pos);
        break;
      }
    }
    table->table_mutex.unlock();
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

    if (table_type.compare(ta::partition_string) == 0)
      table = new Partition_table(name);
    else
      table = new Table(name);

    for (auto &col : tab["columns"].GetArray()) {
      Column *a =
          new Column(col["name"].GetString(), col["type"].GetString(),
                     col["null"].GetBool(), col["lenght"].GetInt(), table);
      table->AddColumn(a);
    }

    for (auto &ind : tab["indexes"].GetArray()) {
      Index *index = new Index(ind["name"].GetString());

      for (auto &ind_col : ind["index_columns"].GetArray()) {
        string index_base_column = ind_col["name"].GetString();

        for (auto &column : *table->columns_) {
          if (index_base_column.compare(column->name_) == 0) {
            index->AddColumn(new Ind_col(column, ind_col["desc"].GetBool()));
            break;
          }
        }
      }
      table->AddIndex(index);
    }

    all_tables->push_back(table);
    ta::no_of_tables = all_tables->size();
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
  auto conn = thd->conn;
  std::random_device dev;
  std::mt19937 rng(dev());
  logs << " creating defaut tables " << endl;

  ta::load_from_file == 1 ? load_objects_from_file(all_tables)
                          : create_default_tables(all_tables);

  if (ta::no_of_tables <= 0)
    throw std::runtime_error("no table to work on \n");

  execute_sql("drop database test;", conn);
  execute_sql("create database test;", conn);
  execute_sql("use test;", conn);
  for (auto &table : *all_tables) {
    execute_sql(table->Table_defination(), conn);
  }
  load_default_data(all_tables, conn);
  return 1;
}

void run_some_query(Thd1 *thd) {
  auto &logs = thd->thread_log;
  auto *conn = thd->conn;
  auto all_tables = thd->tables;
  logs << "executing use test " << endl;
  execute_sql("use test;", conn);
  Node::parallel_thread_running++;
  for (int i = 0; i < 40; i++) {
    auto size = all_tables->size();

    if (i % 20 == 0)
      logs << " executed " << i << " queries " << std::endl;

    auto table = all_tables->at(rand_int(size - 1));
    auto x = rand_int(6);
    switch (x) {
    case 0:
      at_drop_column(table, conn);
      break;
    case 1:
      at_add_column(table, conn);
      break;
    case 2:
      insert_data(table, conn);
      break;
    case 3:
      table->Analyze(thd);
      break;

    case 4:
      table->Optimize(thd);
      break;
    case 5:
      table->DropCreate(thd);
      break;
    case 6:
      table->Truncate(thd);
      break;
    }
  }
  Node::parallel_thread_running--;
}

