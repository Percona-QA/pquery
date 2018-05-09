#include <iostream>
#include <string>
#include <stdexcept>
#include <iomanip>
#include <hCommon.hpp>
#include <cLogger.hpp>

Logger::Logger() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  std::ios_base::sync_with_stdio(false);
  }


Logger::~Logger() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if(logFile.is_open()) {
    logFile.close();
    }
  }


void
Logger::addSeparation(char what, int lenght) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  std::ios_base::fmtflags f(logFile.flags());
  logFile << std::setfill(what) << std::setw (lenght) << "\n";
  logFile.flags(f);

  }


bool
Logger::initLogFile(std::string filePath) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if(logFile.is_open()) {
    logFile.close();
    if(logFile.is_open()) {
      std::cerr << "Can't open log file: " << std::string(strerror(errno)) << std::endl;
      return false;
      }
    }
  logFile.open (filePath, std::ios::trunc);
  if(!logFile.is_open()) {
    std::cerr << "Can't open log file: " << std::string(strerror(errno)) << std::endl;
    return false;
    }
  return true;
  }


void
Logger::setPrecision(int precision) {
  logFile.precision(precision);
  logFile << std::fixed;
  }


void
Logger::flushLog() {
  if(!logFile.is_open()) {
    std::cerr << "Log file is not open, can't flush()" << std::endl;
    return;
    }
  logFile.flush();
  if(logFile.fail()) {
    throw std::runtime_error("Can't flush() log file: " + std::string(strerror(errno)));
    }
  }


void
Logger::addRecordToLog(std::string message) {
  logFile << message << "\n";
  if(logFile.fail()) {
    throw std::runtime_error("Can't write to log file: " + std::string(strerror(errno)));
    }
  }


void
Logger::addPartialRecord(std::string message) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  logFile << message;
  if(logFile.fail()) {
    throw std::runtime_error("Can't write to log file: " + std::string(strerror(errno)));
    }
  }
