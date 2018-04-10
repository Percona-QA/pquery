#ifndef PQUERY_HPP
#define PQUERY_HPP

#include <string>
#include <INIReader.hpp>

class PQuery
  {
  public:
    PQuery();
    ~PQuery();
    int run();
    bool initConfig();
    bool parseCliOptions(int argc, char* argv[]);
    void showHelp();
    void showVersion();
    void setConfigFilePath(std::string path) { configFilePath = path; }
  private:
    std::string configFilePath;
    INIReader * configReader;
  };
#endif
