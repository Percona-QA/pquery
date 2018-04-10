#ifdef DEBUG
  #include <iostream>
#endif

#include <cLogger.hpp>

Logger::Logger() {
  #ifdef DEBUG
    std::cerr << __PRETTY_FUNCTION__ << std::endl;
  #endif
  LogEvents = LOG_NOTHING;
  }


Logger::~Logger() {
  #ifdef DEBUG
    std::cerr << __PRETTY_FUNCTION__ << std::endl;
  #endif
    if(logFile.is_open()){
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

void
Logger::setLogFilePath(std::string file) {
  #ifdef DEBUG
    std::cerr << __PRETTY_FUNCTION__ << std::endl;
  #endif
  logFilePath = file;
}

logVerbosity
Logger::getLogVerbosity() {
  #ifdef DEBUG
    std::cerr << __PRETTY_FUNCTION__ << std::endl;
  #endif
  return LogEvents;
}

bool
Logger::initLogFile(){
  #ifdef DEBUG
    std::cerr << __PRETTY_FUNCTION__ << std::endl;
  #endif
  return initLogFile(logFilePath);
}

bool
Logger::initLogFile(std::string filePath){
  #ifdef DEBUG
    std::cerr << __PRETTY_FUNCTION__ << std::endl;
  #endif
  if(logFile.is_open()){
      logFile.close();
      if(logFile.is_open()){
        std::cerr << "Can't close log file!" << std::endl;
        return false;
      }
  }
  logFile.open (filePath);
  if(!logFile.is_open()){
    std::cerr << "Can't open log file!" << std::endl;
    return false;
  }

  return true;
}
