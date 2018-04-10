#ifndef PQUERY_HPP
#define PQUERY_HPP

#include <string>


class PQuery
  {
  public:
    int run();
    bool parseCliOptions(int argc, char* argv[]);
    void showHelp();
    void showVersion();
    void setConfigFilePath(std::string path) { configFilePath = path; }
  private:
    std::string configFilePath;
  };
#endif
