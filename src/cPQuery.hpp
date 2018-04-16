#ifndef PQUERY_HPP
#define PQUERY_HPP

#include <string>
#include <algorithm>
#include <INIReader.hpp>
#include <cLogger.hpp>
#include <cWorker.hpp>

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

    inline std::string toLowerCase(std::string str) {
      auto lowercased = str;
      std::transform (lowercased.begin(), lowercased.end(), lowercased.begin(), ::tolower);
      return lowercased;
      }

    wRETCODE createWorkerWithParams(std::string);
    void setupWorkerParams(struct workerParams&, std::string);
    wRETCODE createWorkerProcess(struct workerParams&);
//
    INIReader* configReader;
    Logger* pqLogger;

  };
#endif
