#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <random>

#include <sWorkerParams.hpp>
#include <cLogger.hpp>
#include <cDatabase.hpp>

#ifndef PQDBWORKER_HPP
#define PQDBWORKER_HPP

const uint16_t MAX_CON_FAILURES = 250;

class DbWorker
  {

  public:
    DbWorker();
    virtual ~DbWorker();
    bool executeTests(struct workerParams);
    void setupLogger(std::shared_ptr<Logger>);
    bool loadQueryList();
    virtual std::shared_ptr<Database> createDbInstance() = 0;
    virtual void endDbThread() = 0;

  protected:
    void workerThread(int);
    void adjustRuntimeParams();
    void spawnWorkerThreads();
    std::vector<std::thread> workers;
    std::shared_ptr<Logger> wLogger;
    std::shared_ptr<std::vector<std::string>> queryList;
    struct workerParams mParams;

  private:
    void writeFinalReport();
    virtual bool testConnection() = 0;
    void storeParams(struct workerParams& wParams);
    bool isComment(std::string&);
    std::atomic <uint64_t> performed_queries_total;
    std::atomic <uint64_t> failed_queries_total;
    std::random_device rd;
  };
#endif
