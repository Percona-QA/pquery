#ifndef __NODE_HPP__
#define __NODE_HPP__

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include "pquery.hpp"
#include <mysql.h>

/* struct for node setup */
struct
workerParams
  {
  std::string myName;                             // unique name for worker
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
  };

/*
It represents standalone MySQL server or MySQL node in cluster (PXC) setup
*/

class Node
  {
  public:
    Node();
    ~Node();
    void setAllParams(struct workerParams& Params) { myParams = Params; }
    int startWork();
  private:
// declaration for worker thread function
    void workerThread(int);
    inline unsigned long long getAffectedRows(MYSQL*);
    void tryConnect();
    bool createGeneralLog();
    void readSettings(std::string);
    void writeFinalReport();

    std::vector<std::thread> workers;
    std::vector<std::string> * querylist;
    struct workerParams myParams;
    std::ofstream general_log;
    std::atomic <unsigned long long> performed_queries_total;
    std::atomic <unsigned long long> failed_queries_total;
  };
#endif

int
Node::startWork() {

  workers.resize(myParams.threads);

  for (int i=0; i<myParams.threads; i++) {
    workers[i] = std::thread(&Node::workerThread, this, i);
    }

  for (int i=0; i<myParams.threads; i++) {
    workers[i].join();
    }
  return EXIT_SUCCESS;
  }
