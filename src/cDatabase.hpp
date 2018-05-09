#include <cstdint>
#include <memory>
#include <chrono>
#include <sWorkerParams.hpp>

#ifndef _DATABASE_HPP_
#define _DATABASE_HPP_

class Database
  {
  public:
    Database();
    virtual ~Database();
    virtual std::string getServerVersion() = 0;
    virtual std::string getHostInfo() = 0;
    virtual std::string getErrorString() = 0;
    virtual bool init() = 0;
    virtual bool connect(struct workerParams&) = 0;
    virtual std::uint64_t getAffectedRows() = 0;
    double getQueryDurationMs();
    std::uint64_t getPerformedQueries() { return performed_queries; }
    std::uint64_t getFailedQueries() { return failed_queries; }
    std::uint16_t getConsecutiveFailures() { return max_con_fail_count; }
    bool performQuery(std::string);
    std::string getQueryResult() { return queryResult; }
    virtual bool performRealQuery(std::string) = 0;
    virtual void processQueryOutput() = 0;
    virtual std::uint32_t getWarningsCount() = 0;

  protected:
    std::string queryResult;

  private:
    std::chrono::steady_clock::time_point begin;
    std::chrono::steady_clock::time_point end;
    std::uint64_t failed_queries;
    std::uint64_t performed_queries;
    std::uint16_t max_con_fail_count;

  };
#endif
