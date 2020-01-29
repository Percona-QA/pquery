#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#ifndef PQVERSION
#define PQVERSION "3"
#endif
#ifdef MAXPACKET
  #ifndef MAX_PACKET_DEFAULT
  #define MAX_PACKET_DEFAULT 4194304
  #endif
#endif

#ifndef FORK
#define FORK "MySQL"
#endif

#ifndef PQREVISION
#define PQREVISION "unknown"
#endif
#include <getopt.h>
#include <atomic>
#include <map>
#include <string>
#include <algorithm>
#include <vector>

struct Option {
  enum Type { BOOL, INT, STRING } type;
  enum Opt {
    INITIAL_SEED,
    NUMBER_OF_GENERAL_TABLESPACE,
    NUMBER_OF_UNDO_TABLESPACE,
    UNDO_SQL,
    ENGINE,
    JUST_LOAD_DDL,
    NO_DDL,
    ONLY_CL_DDL,
    ONLY_CL_SQL,
    NO_ENCRYPTION,
    ENCRYPTION_TYPE,
    NO_COLUMN_COMPRESSION,
    NO_TABLE_COMPRESSION,
    NO_TABLESPACE,
    NO_BLOB,
    NO_VIRTUAL_COLUMNS,
    TABLES,
    INDEXES,
    ALGORITHM,
    LOCK,
    COLUMNS,
    INDEX_COLUMNS,
    NO_AUTO_INC,
    NO_DESC_INDEX,
    NO_TEMPORARY,
    ONLY_TEMPORARY,
    TEMPORARY_TO_NORMAL_RATIO,
    INITIAL_RECORDS_IN_TABLE,
    NUMBER_OF_SECONDS_WORKLOAD,
    ALTER_TABLE_ENCRYPTION,
    ALTER_TABLE_COMPRESSION,
    ALTER_COLUMN_MODIFY,
    PRIMARY_KEY,
    ROW_FORMAT,
    SERVER_OPTION_FILE,
    SET_GLOBAL_VARIABLE,
    ALTER_MASTER_KEY,
    ROTATE_REDO_LOG_KEY,
    ALTER_TABLESPACE_ENCRYPTION,
    ALTER_TABLESPACE_RENAME,
    ALTER_DATABASE_ENCRYPTION,
    NO_SELECT,
    NO_INSERT,
    NO_UPDATE,
    NO_DELETE,
    SELECT_ALL_ROW,
    SELECT_ROW_USING_PKEY,
    INSERT_RANDOM_ROW,
    UPDATE_ROW_USING_PKEY,
    DELETE_ALL_ROW,
    DELETE_ROW_USING_PKEY,
    DROP_COLUMN,
    ADD_COLUMN,
    DROP_INDEX,
    ADD_INDEX,
    RENAME_COLUMN,
    OPTIMIZE,
    ANALYZE,
    TRUNCATE,
    DROP_CREATE,
    INVALID_OPTION = 63,
    LOG_ALL_QUERIES = 'A',
    DYNAMIC_PQUERY = 'k',
    DATABASE = 'd',
    ADDRESS = 'a',
    INFILE = 'i',
    LOGDIR = 'l',
    SOCKET = 's',
    CONFIGFILE = 'c',
    PORT = 'p',
    PASSWORD = 'P',
    NO_SHUFFLE = 'n',
    THREADS = 't',
    LOG_FAILED_QUERIES = 'F',
    LOG_SUCCEDED_QUERIES = 'S',
    LOG_QUERY_STATISTICS = 'L',
    LOG_QUERY_DURATION = 'D',
    LOG_QUERY_NUMBERS = 'N',
    LOG_CLIENT_OUTPUT = 'O',
    TEST_CONNECTION = 'T',
    QUERIES_PER_THREAD = 'q',
    USER = 'u',
    HELP = 'h',
    VERBOSE = 'v',
    MYSQLD_SERVER_OPTION = 'z',
    TRANSATION_PRB_K,
    TRANSACTIONS_SIZE,
    COMMMIT_TO_ROLLBACK_RATIO,
    SAVEPOINT_PRB_K,
    STEP,
    METADATA_PATH,
    SPECIAL_SQL,
    SQL_FILE,
    MAX
  } option;
  Option(Type t, Opt o, std::string n)
      : type(t), option(o), name(n), sql(false), ddl(false), total_queries(0),
        success_queries(0){};
  ~Option();

  void print_pretty();
  Type getType() { return type; };
  Opt getOption() { return option; };
  const char *getName() { return name.c_str(); };
  bool getBool() { return default_bool; }
  int getInt() { return default_int; }
  std::string getString() { return default_value; }
  short getArgs() { return args; }
  void setArgs(short s) { args = s; };
  void setBool(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    if (s.compare("ON") == 0 || s.compare("TRUE") == 0 || s.compare("1") == 0)
      default_bool = true;
    else if (s.compare("OFF") == 0 || s.compare("FALSE") == 0 ||
             s.compare("0") == 0)
      default_bool = false;
    else {
      // todo throw some execption
    }
  }

  void setBool(bool in) {
    default_bool = in;
  }
  void setInt(std::string n) {
    default_int = stoi(n);
  }
  void setInt(int n) {
    default_int = n;
  }
  void setString(std::string n) {
    default_value = n;
  };
  void setSQL() { sql = true; };
  void setDDL() { ddl = true; };
  void set_cl() { cl = true; }

  std::string name;
  std::string help;
  std::string default_value;
  int default_int; // if default value is integer
  bool default_bool; // if default value is bool
  bool sql; // true if option is SQL, False if others
  bool ddl; // If SQL is DDL, or false if it is not
  bool cl = false;                // set if it was pass trough command line
  short args = required_argument; // default is required argument
  std::atomic<unsigned long int> total_queries;   // totatl times executed
  std::atomic<unsigned long int> success_queries; // successful count
};

struct Server_Option { // Server_options
  Server_Option(std::string n) : name(n){};
  int prob;
  std::string name;
  std::vector<std::string> values;
};

/* delete options and server_options*/
void delete_options();
typedef std::vector<Option *> Opx;
typedef std::vector<Server_Option *> Ser_Opx;
extern Opx *options;
extern Ser_Opx *server_options;
void add_options();
void add_server_options(std::string str);
void add_server_options_file(std::string file_name);
Option *newOption(Option::Type t, Option::Opt o, std::string s);

#endif
