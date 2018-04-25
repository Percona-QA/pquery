#include <string>
#include <fstream>

#ifndef LOGGER_HPP
#define LOGGER_HPP

enum logVerbosity
  {
  LOG_NOTHING           = 0,
  LOG_MINIMUM           = 1 << 0,
  LOG_QUERY_NUMBERS     = 1 << 1,
  LOG_CLIENT_OUTPUT     = 1 << 2,
  LOG_QUERY_DURATION    = 1 << 3,
  LOG_QUERY_STATISTICS  = 1 << 4,
  LOG_FAILED_QUERIES    = 1 << 5,
  LOG_SUCCEDED_QUERIES  = 1 << 6,
  LOG_ALL_QUERIES       = LOG_FAILED_QUERIES | LOG_SUCCEDED_QUERIES,
  };

class Logger
  {

  public:
    Logger();
    ~Logger();
    logVerbosity getLogVerbosity();
    void setLogVerbosity(logVerbosity);
    void setLogFilePath(std::string);
    bool initLogFile(std::string);
    void addRecordToLog(std::string);
    void addRecordToLog(std::string, logVerbosity);
    void flushLog();

  private:
    logVerbosity LogEvents;
    std::ofstream logFile;

  };
#endif
