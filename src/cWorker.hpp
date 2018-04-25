#ifndef PQWORKER_HPP
#define PQWORKER_HPP

#include <string>

struct
workerParams
  {
  std::string myName;                             // unique name for worker
  std::string database;
  std::string dbtype;
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
  };

class Worker
  {
  public:
    Worker(struct workerParams);
    int startWork(struct workerParams);
    bool tryConnect();

  };
#endif
