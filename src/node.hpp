#ifndef __NODE_HPP__
#define __NODE_HPP__

#include "pquery.hpp"
#include "random_test.hpp"
#include <mysql.h>
#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

/* struct for node setup */
struct workerParams {
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
  unsigned long maxpacket;
  bool verbose;
  bool debug;
  bool log_all_queries;
  bool log_failed_queries;
  bool log_query_statistics;
  bool log_query_duration;
  bool log_client_output;
  bool log_query_numbers;
  bool log_succeeded_queries;
  bool shuffle;
  bool test_connection;
  bool ddl;
};

enum LogLevel {
  LOG_NOTHING = 0,
  LOG_QUERY_NUMBERS = 1 << 0,
  LOG_CLIENT_OUTPUT = 1 << 1,
  LOG_QUERY_DURATION = 1 << 2,
  LOG_QUERY_STATISTICS = 1 << 3,
  LOG_FAILED_QUERIES = 1 << 4,
  LOG_SUCCEDED_QUERIES = 1 << 5,
  LOG_ALL_QUERIES = LOG_FAILED_QUERIES | LOG_SUCCEDED_QUERIES,
  LOG_CURRENT = LOG_NOTHING
};

/*
It represents standalone MySQL server or MySQL node in cluster (PXC) setup
*/

class Node {
public:
  Node();
  ~Node();
  void setAllParams(struct workerParams &Params) { myParams = Params; }
  int startWork();

private:
  // declaration for worker thread function
  void workerThread(int);
  void Random_Generated_Load(int);
  inline unsigned long long getAffectedRows(MYSQL *);
  void tryConnect();
  bool createGeneralLog();
  void readSettings(std::string);
  void writeFinalReport();
  void random_Generated_Load(int number);
  std::vector<std::thread> workers;
  std::vector<std::string> *querylist;

  std::vector<Table *> *tables;

  struct workerParams myParams;
  std::ofstream general_log;
  std::atomic<unsigned long long> performed_queries_total;
  std::atomic<unsigned long long> failed_queries_total;

public:
  std::atomic<bool> default_load;
  std::atomic<int> threads_create_table;
};
#endif
