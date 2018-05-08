#include <memory>
#include <chrono>
#include <sWorkerParams.hpp>

#ifndef _DATABASE_HPP_
#define _DATABASE_HPP_

class Database
  {
  public:
    virtual ~Database();
    virtual std::string getServerVersion() = 0;
    virtual std::string getHostInfo() = 0;
    virtual std::string getErrorString() = 0;
    virtual bool init() = 0;
    virtual bool connect(struct workerParams&) = 0;
    virtual unsigned long long getAffectedRows() = 0;
    std::chrono::duration<double> getQueryDuration();

  protected:
    std::chrono::steady_clock::time_point begin;
    std::chrono::steady_clock::time_point end;
    int failed_queries = 0;
    int total_queries = 0;
    int max_con_fail_count = 0;

  };
#endif
