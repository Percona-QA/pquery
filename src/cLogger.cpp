
#include <iostream>

#include <cstring>
#include <string>
#include <cerrno>
#include <stdexcept>
#include <common.hpp>
#include <cLogger.hpp>

Logger::Logger() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  LogEvents = LOG_NOTHING;
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
Logger::setLogVerbosity(logVerbosity level) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  LogEvents = level;
  }


logVerbosity
Logger::getLogVerbosity() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  return LogEvents;
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
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  logFile << message << "\n";
  if(logFile.fail()) {
    throw std::runtime_error("Can't write to log file: " + std::string(strerror(errno)));
    }
  }


void
Logger::addRecordToLog(std::string message, logVerbosity verbosity) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if (LogEvents & verbosity) {
    addRecordToLog(message);
    }
  }
