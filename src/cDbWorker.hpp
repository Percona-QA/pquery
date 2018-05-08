#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>

#include <sWorkerParams.hpp>
#include <cLogger.hpp>
#include <cDatabase.hpp>


#ifndef PQDBWORKER_HPP
#define PQDBWORKER_HPP

class DbWorker
  {

  public:
    DbWorker();
    virtual ~DbWorker();
    bool executeTests(struct workerParams);
    void setupLogger(std::shared_ptr<Logger>);
    bool loadQueryList();
    virtual std::shared_ptr<Database> createDbInstance() = 0;
    virtual void endThread() = 0;

  protected:
    void workerThread(int);
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
    virtual bool testConnection() = 0;
    void storeParams(struct workerParams& wParams);
    bool isComment(std::string&);

  };
#endif
