#include <string>
#include <cLogger.hpp>
#include <eDbTypes.hpp>

#ifndef PQDBWORKER_HPP
#define PQDBWORKER_HPP

struct
workerParams
  {
  std::string myName;                             // unique name for worker
  std::string database;
  eDBTYPE dbtype;
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

class DbWorker
  {
  public:
    DbWorker();
    ~DbWorker();
    int executeTests(struct workerParams);
    bool tryConnect();
    void setupLogger(std::shared_ptr<Logger>);
  private:
    bool testConnection();
    void storeParams(struct workerParams wParams);
    struct workerParams mParams;
    std::shared_ptr<Logger> wLogger;

  };
#endif
