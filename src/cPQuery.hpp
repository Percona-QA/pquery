#ifndef PQUERY_HPP
#define PQUERY_HPP

#include <string>
#include <INIReader.hpp>
#include <cLogger.hpp>

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
    void showHelp();
    void showVersion();
    void setConfigFilePath(std::string configPath) { configFilePath = configPath; }
    void setLogFilePath(std::string logPath) { logFilePath = logPath; }
    bool logVersionInfo();

  private:
    std::string configFilePath;
    std::string logFilePath;

    INIReader* configReader;
    Logger* pqLogger;
  };
#endif
