#include <cstdio>
#include <document.h>
#include <filereadstream.h>
#include <fstream>
#include <iostream>
#include <prettywriter.h>
#include <random>
#include <string.h>
#include <vector>
#include <writer.h>

using namespace std;
using namespace rapidjson;
std::mt19937 rng;

int rand_int(int upper, int lower = 0) {
  /*todo change the approach if it is too slow */
  std::uniform_int_distribution<std::mt19937::result_type> dist(
      lower, upper); // distribution in range [lower, upper]
  return dist(rng);
}

template <typename Writer>
void key_value(Writer writer, std::string key, std::string value) {
  writer.StartObject();
  writer.String(key.c_str(), static_cast<SizeType>(key.length()));
  writer.String(value.c_str(), static_cast<SizeType>(value.length()));
  writer.EndObject();
}

/* table attirbutes */
namespace ta {
string encryption;
vector<string> key_block_size;
string engine = "InnoDB";
vector<string> tablespace = {"abc"};
int no_of_tables = 100;
int version = 1;
string file_path = "/Users/rahulmalik/test_tool/data.dll";
int load_from_file = 0;
string partition_string = "_p";
int default_columns_in_table = 10;
int default_indexes_in_table = 5;
int default_columns_in_index = 7;
int default_number_of_records_in_table = 5;
}; // namespace ta

/* Different table type supported by tool */
enum TABLE_TYPES { PARTITION, NORMAL };
/* Column Basic Properties */

struct Column {
  Column(string n) : name_(n){};
  Column(string name, string type, bool n)
      : name_(name), type_(type), null(n){};
  std::string name_;
  std::string type_;
  int length = 0;
  bool null = false;
  std::string default_value;
  bool primary_key;
  bool generated;
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
    writer.EndObject();
  }
};

struct Ind_col {
  Ind_col(Column *c) : column(c){};
  Ind_col(Column *c, bool d) : column(c), desc(d){};
  Column *column;
  bool desc = false;
  int length = 0;
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
};

class Index {

public:
  Index(string n) : columns_(), name_(n){};
  vector<Ind_col> columns_;
  void AddColumn(Ind_col column) { columns_.push_back(column); }
  template <typename Writer> void Serialize(Writer &writer) const {
    writer.StartObject();
    writer.String("name");
    writer.String(name_.c_str(), static_cast<SizeType>(name_.length()));
    writer.String(("index_columns"));
    writer.StartArray();
    for (auto ic = columns_.begin(); ic != columns_.end(); ic++)
      ic->Serialize(writer);
    writer.EndArray();
    writer.EndObject();
  }

  std::string name_;
};

/* Table basic properties */
struct Table {
public:
  string engine;
  TABLE_TYPES table_type;
  Table(std::string n) : name_(n), indexes_() {
    columns_ = new vector<Column *>;
  };
  std::string table_name() { return name_; };
  /* method to create table of choice */
  static Table *table_id(int choice, int id);
  string table_defination();

  void AddColumn(Column *column) { columns_->push_back(column); }

  void AddIndex(const Index &index) { indexes_.push_back(index); }

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
    for (auto ind = indexes_.begin(); ind != indexes_.end(); ind++)
      ind->Serialize(writer);
    writer.EndArray();
    writer.EndObject();
  }

  virtual ~Table() {
    for (auto col : *columns_)
      delete col;
    delete columns_;
  };

  std::string name_;
  bool encryption;
  std::string key_block_size;
  std::string data_directory;
  std::string compression;
  std::string tablespace;
  std::vector<Column *> *columns_;
  std::vector<Index> indexes_;
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
void create_default_colum(Table *table) {
  int j = rand_int(ta::default_columns_in_table, 1);
  cout << j << endl;
  for (int i = 0; i < j; i++) {
    string name = "a" + to_string(i);
    Column *a = new Column{name}; // = Column(name);
    a->type_ = "INT";
    a->null = rand_int(2);
    if (rand_int(10) == 1)
      a->length = rand_int(100, 20);
    a->default_value = rand_int(100);
    table->AddColumn(a);
  }
}

/* create default indexes */
void create_default_index(Table *table) {

  int indexes = rand_int(ta::default_indexes_in_table, 1);
  for (int i = 0; i < indexes; i++) {

    Index id = Index(table->name_ + "i" + to_string(i));

    int max_columns = rand_int(ta::default_columns_in_index, 1);

    int columns = max_columns;

    for (auto *col : *table->columns_) {
      if (rand_int(3) >= 4)
        continue;
      else {
        if (rand_int(2) > 0)
          id.AddColumn(Ind_col(col)); // desc is set false
        else
          id.AddColumn(Ind_col(col, true)); // desc is set as true
        if (--columns == 0)
          break;
      }
    }

    /* make sure it has  atleast one column */
    if (columns == max_columns)
      id.AddColumn(Ind_col(table->columns_->at(0)));
    table->AddIndex(id);
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

  create_default_colum(table);
  create_default_index(table);

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
string Table::table_defination() {
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

  for (auto id = indexes_.begin(); id != indexes_.end(); id++) {
    def += "INDEX " + id->name_ + "(";
    for (auto idc = id->columns_.begin(); idc != id->columns_.end(); idc++) {
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
vector<Table *> *create_default_tables() {
  int no_of_tables = ta::no_of_tables;
  vector<Table *> *all_tables = new vector<Table *>;
  for (int i = 0; i < no_of_tables; i++) {
    Table *t = Table::table_id(rand_int(2), i);
    // auto *n = static_cast<Partition_table *>(t);
    all_tables->push_back(t);
  }
  return all_tables;
}

void load_default_data(vector<Table *> *all_tables) {
  for (auto i = all_tables->begin(); i != all_tables->end(); i++) {
    auto table = *i;
    int rec = rand_int(ta::default_number_of_records_in_table);
    for (int i = 0; i <= rec; i++) {
      string vals = "";
      string insert = "INSERT INTO " + table->name_ + "  ( ";
      for (auto &column : *table->columns_) {
        insert += column->name_ + " ,";
        vals += " " + to_string(rand_int(100)) + ",";
      }
      vals.pop_back();
      insert.pop_back();
      insert += ") VALUES(" + vals;
      insert += " );";
      cout << insert << endl;
    }
  }
}

/* alter table add column */
void at_add_column(Table *table) {
  auto col_name = "COL" + to_string(rand_int(3));
  string sql = "ALTER TABLE " + table->name_ + "  ADD COLUMN ";
  Column *tc = new Column(col_name);
  tc->type_ = "INT";
  table->AddColumn(tc);
  sql += col_name + " (INT); ";
  cout << sql << endl;
}

/* alter table drop column */
void at_drop_column(Table *table) {
  if (table->columns_->size() == 1)
    return;
  auto pos = rand_int(table->columns_->size() - 1);
  auto &col = table->columns_->at(pos);
  string sql =
      "ALTER TABLE " + table->name_ + " DROP COLUMN " + col->name_ + ";";
  cout << sql << endl;
  table->columns_->erase(table->columns_->begin() + pos);
}


/* save objects to a file */
void save_objects_to_file(vector<Table *> *all_tables, int version) {
  //  rapidjson::Writer<Stream> writer(stream);

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
  std::ofstream of(ta::file_path);
  of << sb.GetString();
  if (!of.good())
    throw std::runtime_error("Can't write the JSON string to the file!");
}

/*load objects from a file */
vector<Table *> *load_objects_from_file() {
  int version = ta::version;
  vector<Table *> *all_tables = new vector<Table *>;
  FILE *fp = fopen(ta::file_path.c_str(), "r");
  char readBuffer[65536];
  FileReadStream is(fp, readBuffer, sizeof(readBuffer));
  Document d;
  d.ParseStream(is);
  auto v = d["version"].GetInt();

  if (d["version"].GetInt() != version)
    throw std::runtime_error("version mismatch between " + ta::file_path +
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
      Column *a = new Column(col["name"].GetString(), col["type"].GetString(),
                             col["null"].GetBool());
      table->AddColumn(a);
    }

    for (auto &ind : tab["indexes"].GetArray()) {
      Index index = Index(ind["name"].GetString());

      for (auto &ind_col : ind["index_columns"].GetArray()) {
        string index_base_column = ind_col["name"].GetString();

        for (auto &column : *table->columns_) {
          if (index_base_column.compare(column->name_) == 0) {
            index.AddColumn(Ind_col(column, ind_col["desc"].GetBool()));
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

  return all_tables;
}

/* clean tables from memory */
void clean_up_at_end(vector<Table *> *all_tables) {
  for (auto &table : *all_tables)
    delete table;
  delete all_tables;
}

/*
int main() {
  std::random_device dev;
  std::mt19937 rng(dev());

  vector<Table *> *all_tables =
      ta::load_from_file ? load_objects_from_file() : create_default_tables();

  if (ta::no_of_tables <= 0)
    throw std::runtime_error("no table to work on \n");

  //  load_default_data(all_tables);

  for (int i = 0; i < 1000; i++) {
    at_add_column(all_tables->at(rand_int(ta::no_of_tables - 1)));

    at_drop_column(all_tables->at(rand_int(ta::no_of_tables - 1)));
  }

  load_default_data(all_tables);

  for (auto &table : *all_tables) {
    std::cout << table->table_defination() << endl;
  }

  if (!ta::load_from_file)
    save_objects_to_file(all_tables, ta::version);

  clean_up_at_end(all_tables);

  return (0);
}
*/
