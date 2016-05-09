#ifndef __PQUERY_HPP__
#define __PQUERY_HPP__

#include <vector>
#include <string>
#include "node.hpp"

class Node;

// declaration for help functions
void print_version(void);
void show_help(void);
void show_config_help(void);
void show_cli_help(void);

/* struct for node setup */
struct 
workerParams {
  std::string myName; // unique name for worker
  std::string database;
  std::string address;
  std::string socket;
  std::string username;
  std::string password;
  std::string infile;
  std::string logdir;
  short port;
  short threads;
  unsigned long queries_per_thread;
  bool verbose;
  bool debug;
  bool log_all_queries;
  bool log_failed_queries;
  bool log_query_statistics;
  bool log_query_duration;
  bool log_client_output;
  bool log_query_numbers;
  bool shuffle;
  bool test_connection;
};

// declaration for (re)setting defaults
void 
set_defaults(struct workerParams&);

void
read_section_settings(struct workerParams&, std::string, std::string);

void 
create_worker(struct workerParams&);

#ifndef PQVERSION
#define PQVERSION "2.0"
#endif

#ifndef FORK
#define FORK "MySQL"
#endif
#endif
