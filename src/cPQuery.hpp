#include <string>
#include <algorithm>
#include <INIReader.hpp>
#include <cLogger.hpp>
#include <cDbWorker.hpp>

#ifndef PQUERY_HPP
#define PQUERY_HPP

enum wRETCODE
  {
  wDEFAULT,
  wMASTER,
  wCHILD,
  wERROR
  };

class PQuery
  {

  public:
    PQuery();
    ~PQuery();
    bool prepareToRun();
    int run();
    bool initConfig();
    bool initLogger();
    bool parseCliOptions(int argc, char* argv[]);
    bool runWorkers();
    void showHelp();
    void showVersion();
    void setConfigFilePath(std::string configPath) { configFilePath = configPath; }
    void setLogFilePath(std::string logPath) { logFilePath = logPath; }
    bool logVersionInfo();

  private:
    std::string configFilePath;
    std::string logFilePath;

    inline std::string
    toLowerCase(std::string str) {
      auto lowercased = str;
      std::transform (lowercased.begin(), lowercased.end(), lowercased.begin(), ::tolower);
      return lowercased;
      }
    inline std::string
    dbtype_str(eDBTYPE type) {
      switch (type) {
        case eMYSQL:
          return "MySQL";
        case ePGSQL:
          return "PostgreSQL";
        case eMONGO:
          return "MongoDB";
        default:
          return "UNKNOWN";
        }
      }
    void  doCleanup(std::string);
    void logWorkerDetails(struct workerParams&);

#ifdef HAVE_MYSQL
    std::string getMySqlClientInfo();
#endif
#ifdef HAVE_PGSQL
    std::string getPgSqlClientInfo();
#endif
#ifdef HAVE_MONGO
    std::string getMongoDBClientInfo();
#endif

    wRETCODE createWorkerWithParams(std::string);
    void setupWorkerParams(struct workerParams&, std::string);
    wRETCODE createWorkerProcess(struct workerParams&);
//
    std::shared_ptr<INIReader> configReader;
    std::shared_ptr<Logger> pqLogger;
    std::shared_ptr<DbWorker> dbWorker;

  };
#endif
