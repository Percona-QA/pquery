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
    template<typename T>
    Logger& operator<<(const T& rhs) {
      logFile << rhs;
      if(logFile.fail()) {
        throw std::runtime_error("Can't write to log file: " + std::string(strerror(errno)));
        }
      return(*this);
      }

    void setLogFilePath(std::string);
    bool initLogFile(std::string);
    void addRecordToLog(std::string);
    void addSeparation(char, int);
    void flushLog();
    void setPrecision(int);
    void addPartialRecord(std::string message);

  private:
    std::ofstream logFile;

  };
#endif
