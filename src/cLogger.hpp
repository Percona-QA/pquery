#include <string>
#include <fstream>
#include <bitset>

#ifndef LOGGER_HPP
#define LOGGER_HPP

class Logger
  {

  public:
    Logger();
    ~Logger();
    void setLogFilePath(std::string);
    bool initLogFile(std::string);
    void addRecordToLog(std::string);
    void addSeparation(char, int);
    void flushLog();
    void setPrecision(int);
    void addPartialRecord(std::string);

  private:
    std::ofstream logFile;

  };
#endif
