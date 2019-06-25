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
#include <map>
#include <string>

struct Option {
  enum default_t { BOOL, INT, STRING } default_type;
  enum type {
    DDL,
    TABLE,
    SELECT,
    DROP_COLUMN,
    ADD_COLUMN,
    TRUNCATE,
    DROP_CREATE,
    ENCRYPTION,
    TABLESPACE_ENCRYPTION,
    TABLESPACE_RENAME,
    /*
    VERBOSE,
    COLUMNS,
    INDEXES,
    ADD_COLUMN,
    DROP_INDEX,
    ADD_INDEX,
    */
    MAX
  } option_type;
  Option(default_t t, type j, std::string n, bool s = false, bool d = false)
      : default_type(t), option_type(j), name(n), sql(s), ddl(d){};
  void print_pretty();

  bool getBool() { return default_bool; }

  int getInt() { return default_int; }

  std::string getString() { return default_value; }

  void setBool(std::string in) {
    default_bool = in.compare("true") == 0 ? true : false;
  }

  const char *getName() { return name.c_str(); };
  void setBool(bool in) { default_bool = in; }

  void setInt(std::string n) { default_int = stoi(n); }

  void setInt(int n) { default_int = n; }

  default_t get_type() { return default_type; }

  std::string name;
  std::string help;
  std::string default_value;
  int default_int;
  bool default_bool;
  bool sql; // true if option is SQL, False if others
  bool ddl; // If SQL is DDL, or false if it is not
  ~Option();
};

void delete_options();
extern std::vector<Option *> *options;
std::vector<Option *> *add_options();

#endif
