#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>

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
    virtual ~DbWorker();
    bool executeTests(struct workerParams);
    void setupLogger(std::shared_ptr<Logger>);
    bool loadQueryList();

  protected:
    virtual void workerThread(int) = 0;
    void adjustRuntimeParams();
    void spawnWorkerThreads();
    std::vector<std::thread> workers;
    std::shared_ptr<Logger> wLogger;
    std::shared_ptr<std::vector<std::string>> queryList;
    struct workerParams mParams;
    std::atomic <unsigned long long> performed_queries_total;
    std::atomic <unsigned long long> failed_queries_total;
    std::chrono::steady_clock::time_point begin;
    std::chrono::steady_clock::time_point end;
    std::random_device rd;
  private:
    void writeFinalReport();
    virtual bool testConnection();
    void storeParams(struct workerParams& wParams);
    bool isComment(std::string&);

  };
#endif
