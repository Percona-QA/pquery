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
#include <map>
#include <string>

struct Option {
  enum Type { BOOL, INT, STRING } type;
  enum Opt {
    MODE_OF_PQUERY,
    INITIAL_SEED,
    ENGINE,
    JUST_LOAD_DDL,
    DDL,
    TABLE,
    ENCRYPTION,
    ROW_FORMAT,
    TABLESPACE_ENCRYPTION,
    TABLESPACE_RENAME,
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    SELECT_ALL_ROW,
    SELECT_ROW_USING_PKEY,
    INSERT_RANDOM_ROW,
    UPDATE_ROW_USING_PKEY,
    DELETE_ALL_ROW,
    DELETE_ROW_USING_PKEY,
    DROP_COLUMN,
    ADD_COLUMN,
    RENAME_COLUMN,
    OPTIMIZE,
    ANALYZE,
    TRUNCATE,
    DROP_CREATE,
    DATABASE = 'd',
    ADDRESS = 'a',
    INFILE = 'i',
    LOGDIR = 'l',
    SOCKET = 's',
    CONFIGFILE = 'c',
    PORT = 'p',
    USER = 'u',
    PASSWORD = 'P',
    THREADS = 't',
    HELP = 'z',
    MAX
  } option;
  Option(Type t, Opt o, std::string n)
      : type(t), option(o), name(n), sql(false), ddl(false){};
  void print_pretty();

  Type getType() { return type; };
  Opt getOption() { return option; };

  const char *getName() { return name.c_str(); };
  bool getBool() { return default_bool; }
  int getInt() { return default_int; }
  std::string getString() { return default_value; }
  short getArgs() { return args; }
  void setArgs(short s) { args = s; };

  void setBool(std::string in) {
    default_bool = in.compare("true") == 0 ? true : false;
  }
  void setBool(bool in) { default_bool = in; }
  void setInt(std::string n) { default_int = stoi(n); }
  void setInt(int n) { default_int = n; }
  void setString(std::string n) { default_value = n; };
  void setSQL() { sql = true; };
  void setDDL() { ddl = true; };

  std::string name;
  std::string help;
  std::string default_value;
  int default_int;
  bool default_bool;
  bool sql; // true if option is SQL, False if others
  bool ddl; // If SQL is DDL, or false if it is not
  ~Option();
  short args = required_argument; // default is required argument
};

void delete_options();
typedef std::vector<Option *> Opx;
extern Opx *options;
void add_options();
Option *newOption(Option::Type t, Option::Opt o, std::string s);

#endif
